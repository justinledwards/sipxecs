/*
 * Copyright (c) eZuce, Inc. All rights reserved.
 * Contributed to SIPfoundry under a Contributor Agreement
 *
 * This software is free software; you can redistribute it and/or modify it under
 * the terms of the Affero General Public License (AGPL) as published by the
 * Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 */

#include "sqa/StateQueueConnection.h"
#include "sqa/StateQueueAgent.h"
#include "sqa/StateQueueClient.h"
#include "os/OsLogger.h"


StateQueueConnection::StateQueueConnection(
  boost::asio::io_service& ioService,
  StateQueueAgent& agent) :
  _ioService(ioService),
  _agent(agent),
  _socket(_ioService),
  _resolver(_ioService),
  _moreReadRequired(0),
  _lastExpectedPacketSize(0),
  _localPort(0),
  _remotePort(0),
  _pInactivityTimer(0),
  _isAlphaConnection(false),
  _isCreationPublished(false),
  _isExternalConnection(false),
  _freshRead(false)
{
  int expires = _agent.inactivityThreshold() * 1000;
  _pInactivityTimer = new boost::asio::deadline_timer(_ioService, boost::posix_time::milliseconds(expires));
  OS_LOG_DEBUG(FAC_NET, "StateQueueConnection CREATED.");
}

StateQueueConnection::~StateQueueConnection()
{
  delete _pInactivityTimer;
  stop();
  OS_LOG_DEBUG(FAC_NET, "StateQueueConnection DESTROYED.");
}

bool StateQueueConnection::write(const std::string& data)
{
  short version = 1;
  short key = SQA_KEY_DEFAULT;
  unsigned long len = (short)data.size();
  std::stringstream strm;
  strm.write((char*)(&version), sizeof(version));
  strm.write((char*)(&key), sizeof(key));
  strm.write((char*)(&len), sizeof(len));
  strm << data;
  std::string packet = strm.str();
  boost::system::error_code ec;
  bool ok = _socket.write_some(boost::asio::buffer(packet.c_str(), packet.size()), ec) > 0;
  return ok;
}

void StateQueueConnection::initLocalAddressPort()
{
  try
  {
    if (!_localPort)
    {
      _localPort = _socket.local_endpoint().port();
      _localAddress = _socket.local_endpoint().address().to_string();
    }

    if (!_remotePort)
    {
      _remotePort = _socket.remote_endpoint().port();
      _remoteAddress = _socket.remote_endpoint().address().to_string();
    }
  }
  catch(...)
  {
    //
    //  Exception is non relevant if it is even thrown
    //
  }
}


#define MIN_READ_SIZE (sizeof(short) + sizeof(short) + sizeof(unsigned long))
void StateQueueConnection::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
{
  //close connections on error
  if (e)
  {
    OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
                << "TERMINATED - error:" << e.message()
                << " Most likely remote closed the connection.");
    boost::system::error_code ignored_ec;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    _agent.onDestroyConnection(shared_from_this());
    return;
  }

  // ignore empty read
  if (0 == bytes_transferred)
  {
    OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
                << "TERMINATED - 0 bytes read");
    return;
  }

  //
  // Start the inactivity timer
  //
  startInactivityTimer();

  initLocalAddressPort();
  OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead"
          << " BYTES READ: " << bytes_transferred
          << " SRC: " << _localAddress << ":" << _localPort
          << " DST: " << _remoteAddress << ":" << _remotePort );

  // it is an error to have leftover buffer while in the middle of processing
  if (!_freshRead && _messageBuffer.size())
  {
    // maybe a programming error? recover from error by starting over
    OS_LOG_ERROR(FAC_NET, "StateQueueConnection::handleRead "
        << "TERMINATED - has old data in buffer at new read: " << _messageBuffer);

    _messageBuffer == std::string();
    abortRead();
    return;
  }

  std::stringstream strm;
  //start processing by prepending the remaining buffer
  if (_freshRead && _messageBuffer.size())
  {
    strm.write(_messageBuffer.data(), _messageBuffer.size());
  }
  _freshRead = false;

  // add what was ready now
  strm.write(_buffer.data(), bytes_transferred);

  //check if we have enough data read for a message header, if not read more
  if (strm.str().size() < MIN_READ_SIZE)
  {
    _messageBuffer += std::string(_buffer.data(), bytes_transferred);
    OS_LOG_INFO(FAC_NET,"StateQueueConnection::handleRead "
          << "More bytes required to complete a message header.  "
          << " Read: " << strm.str().size()
          << " Required: " << MIN_READ_SIZE);

    start();
    return;
  }

  short version;
  strm.read((char*)(&version), sizeof(version));

  short key;
  strm.read((char*)(&key), sizeof(key));

  // version and key are valid ?
  if (!(version == 1 && key >= SQA_KEY_MIN && key <= SQA_KEY_MAX))
  {
    //
    // This is a corrupted message.  Simply reset the buffers
    //
    OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
            << "TERMINATED - "
                << " Invalid protocol headers.");
    abortRead();
    return;
  }

  _isAlphaConnection = (key == SQA_KEY_ALPHA);

  unsigned long len;
  strm.read((char*)(&len), sizeof(len));

  //
  // Preserve the expected packet size
  //
  _lastExpectedPacketSize = len + sizeof(version) + sizeof(key) + (sizeof(len));
  if (_lastExpectedPacketSize > SQA_CONN_MAX_READ_BUFF_SIZE)
  {
    //
    // This is a corrupted message.  Simply reset the buffers
    //
    OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
        << "TERMINATED - "
            << " Message exceeds maximum buffer size. Lenth=" << len );
    abortRead();
    return;
  }

  if (_lastExpectedPacketSize > bytes_transferred)
  {
    _messageBuffer += std::string(_buffer.data(), bytes_transferred);
    OS_LOG_INFO(FAC_NET,"StateQueueConnection::handleRead "
          << "More bytes required to complete message.  "
          << " Read: " << bytes_transferred
          << " Required: " << (_lastExpectedPacketSize - bytes_transferred)
          << " Expected: " << _lastExpectedPacketSize
          << " Data: " << len);

    start();
    return;
  }

  //
  // we dont need to read more.  there are enough in the buffer
  //
  char buf[SQA_CONN_MAX_READ_BUFF_SIZE];
  strm.read(buf, len);

  //
  // Check terminating character to avoid corrupted/truncated/overrun messages
  //
  if (buf[len-1] == '_')
  {
    _agent.onIncomingRequest(*this, buf, len -1);

    if (_lastExpectedPacketSize < bytes_transferred)
    {
      //
      // We have spill over bytes
      //
      std::size_t extraBytes = bytes_transferred - _lastExpectedPacketSize;
      char spillbuf[extraBytes];
      strm.read(spillbuf, extraBytes);
      OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead "
          << "Spillover bytes from last read detected.  "
          << " BYTES: " << extraBytes
          << " Buffer: " << _messageBuffer.size()
          << " Expected: " << _lastExpectedPacketSize
          << " Data: " << len
          << " Transferred: " << bytes_transferred);
      memcpy(_buffer.data(), (void*)spillbuf, extraBytes);
      _messageBuffer = std::string();
      handleRead(e, extraBytes);

      return;
    }
    else
    {
      //nothing to do when _lastExpectedPacketSize == bytes_transferred, just read some more
    }
  }
  else // if (buf[len-1] == '_')
  {
    //
    // This is a corrupted message.  Simply reset the buffers
    //
    OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
        << "TERMINATED - "
        << " Message is not terminated with '_' character");
    abortRead();
    return;
  }

  start();

}


#if 0
void StateQueueConnection::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
{
  if (!e && bytes_transferred)
  {

    //
    // Start the inactivity timer
    //
    startInactivityTimer();

    initLocalAddressPort();
    OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead"
            << " BYTES: " << bytes_transferred
            << " SRC: " << _localAddress << ":" << _localPort
            << " DST: " << _remoteAddress << ":" << _remotePort );

    //
    // The first read will always have _moreReadRequired as 0
    //
    if (_moreReadRequired == 0)
    {
      OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead _moreReadRequire==0");

      std::stringstream strm;
      strm.write(_buffer.data(), bytes_transferred);
      OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead strm buffer is:" << strm.str());
      
      short version;
      strm.read((char*)(&version), sizeof(version));

      short key;
      strm.read((char*)(&key), sizeof(key));

      if (version == 1 && key >= SQA_KEY_MIN && key <= SQA_KEY_MAX)
      {
        OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead version ok");
        _isAlphaConnection = (key == SQA_KEY_ALPHA);
        unsigned long len;
        strm.read((char*)(&len), sizeof(len));
        //
        // Preserve the expected packet size
        //
        _lastExpectedPacketSize = len + sizeof(version) + sizeof(key) + (sizeof(len));

        OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead buffer"
            << " _lastExpectedPacketSize: " << _lastExpectedPacketSize
            << " bytes_transferred: " << bytes_transferred
            << " len:" << len);

        if (_lastExpectedPacketSize > bytes_transferred)
          _moreReadRequired =  _lastExpectedPacketSize - bytes_transferred;
        else
          _moreReadRequired = 0;

        //
        // we dont need to read more.  there are enough in the buffer
        //
        if (!_moreReadRequired)
        {
          OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead _moreReadRequire==0");

          if (len < SQA_CONN_MAX_READ_BUFF_SIZE)
          {
            char buf[SQA_CONN_MAX_READ_BUFF_SIZE];
            strm.read(buf, len);

            //
            // Check terminating character to avoid corrupted/truncated/overran messages
            //
            if (buf[len-1] == '_')
            {
              _agent.onIncomingRequest(*this, buf, len -1);

              if (_lastExpectedPacketSize < bytes_transferred)
              {
                //
                // We have spill over bytes
                //
                std::size_t extraBytes = bytes_transferred - _lastExpectedPacketSize;
                char spillbuf[extraBytes];
                strm.read(spillbuf, extraBytes);
                OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead "
                    << "Spillover bytes from last read detected.  "
                    << " BYTES: " << extraBytes
                    << " Buffer: " << _messageBuffer.size()
                    << " Expected: " << _lastExpectedPacketSize
                    << " Data: " << len
                    << " Transferred: " << bytes_transferred);
                memcpy(_buffer.data(), (void*)spillbuf, extraBytes);
                _messageBuffer = std::string();
                handleRead(e, extraBytes);
                return;
              }
            }
            else // if (buf[len-1] == '_')
            {
              //
              // This is a corrupted message.  Simply reset the buffers
              //
              OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
                  << "TERMINATED - "
                  << " Message is not terminated with '_' character");
              OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
                  << "TERMINATED - "
                  << " Message is terminated with: " << buf[len-1]);
              abortRead();
              return;
            }
          }
          else //if (len < SQA_CONN_MAX_READ_BUFF_SIZE)
          {
            //
            // This is a corrupted message.  Simply reset the buffers
            //
            OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
                << "TERMINATED - "
                    << " Message exceeds maximum buffer size. Lenth=" << len );
            abortRead();
            return;
          }
        }
        else // if (!_moreReadRequired)
        {
          _messageBuffer += std::string(_buffer.data(), bytes_transferred);
          OS_LOG_INFO(FAC_NET,"StateQueueConnection::handleRead "
                << "More bytes required to complete message.  "
                << " Read: " << bytes_transferred
                << " Required: " << _moreReadRequired
                << " Expected: " << _lastExpectedPacketSize
                << " Data: " << len);
          OS_LOG_INFO(FAC_NET,"StateQueueConnection::handleRead "
                << "_messageBuffer:  " << _messageBuffer);
        }
      }
      else // if (version == 1 && key >= SQA_KEY_MIN && key <= SQA_KEY_MAX)
      {
        //
        // This is a corrupted message.  Simply reset the buffers
        //
        OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
                << "TERMINATED - "
                    << " Invalid protocol headers.");
        abortRead();
        return;
      }
    }
    else // if (_moreReadRequired == 0)
    {
      readMore(bytes_transferred);
    }
  }
  else if (e) // if (!e && bytes_transferred)
  {
    OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
                << "TERMINATED - " << e.message() 
                << " Most likely remote closed the connection.");
    boost::system::error_code ignored_ec;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    _agent.onDestroyConnection(shared_from_this());
    return;
  }

  start();
}

void StateQueueConnection::readMore(std::size_t bytes_transferred)
{
  if (!bytes_transferred)
    return;

  _messageBuffer += std::string(_buffer.data(), bytes_transferred);

  std::stringstream strm;
  strm << _messageBuffer;
  OS_LOG_WARNING(FAC_NET, "StateQueueConnection::readMode strm buffer is: "
        << strm.str());

  short version;
  strm.read((char*)(&version), sizeof(version));
  short key;
  strm.read((char*)(&key), sizeof(key));

  if (version == 1 && key >= SQA_KEY_MIN && key <= SQA_KEY_MAX)
  {
    OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead version ok");
    unsigned long len;
    strm.read((char*)(&len), sizeof(len));

    _lastExpectedPacketSize = len + sizeof(version) + sizeof(key) + (sizeof(len));

    OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead buffer"
        << " _lastExpectedPacketSize: " << _lastExpectedPacketSize
        << " bytes_transferred: " << bytes_transferred
        << " len:" << len);

    if (_messageBuffer.size() < _lastExpectedPacketSize)
      _moreReadRequired = _lastExpectedPacketSize - _messageBuffer.size();
    else
      _moreReadRequired = 0;

    if (!_moreReadRequired)
    {
      OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::handleRead _moreReadRequire==0");

      if (len < SQA_CONN_MAX_READ_BUFF_SIZE)
      {
        char buf[SQA_CONN_MAX_READ_BUFF_SIZE];
        strm.read(buf, len);

        //
        // Check terminating character to avoid corrupted/truncated/overran messages
        //
        if (buf[len-1] == '_')
        {
          _agent.onIncomingRequest(*this, buf, len -1);

          if (_lastExpectedPacketSize < _messageBuffer.size())
          {
            //
            // We have spill over bytes
            //
            std::size_t extraBytes = _messageBuffer.size() - _lastExpectedPacketSize;
            char spillbuf[extraBytes];
            strm.read(spillbuf, extraBytes);
            OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::readMore "
                << "Spillover bytes from last read detected.  "
                << " BYTES: " << extraBytes
                << " Buffer: " << _messageBuffer.size()
                << " Expected: " << _lastExpectedPacketSize
                << " Data: " << len
                << " Transferred: " << bytes_transferred);
            memcpy(_buffer.data(), (void*)spillbuf, extraBytes);
            boost::system::error_code dummy;
            _messageBuffer = std::string();
            handleRead(dummy, extraBytes);
            return;
          }
        }
        else
        {
          //
          // This is a corrupted message.  Simply reset the buffers
          //
          OS_LOG_WARNING(FAC_NET, "StateQueueConnection::readMode "
              << "TERMINATED - "
              << " Message is not terminated with '_' character");
          OS_LOG_WARNING(FAC_NET, "StateQueueConnection::handleRead "
              << "TERMINATED - "
              << " Message is terminated with: " << buf[len-1]);

          abortRead();
          return;
        }
      }
      else
      {
        //
        // We got an extreme large len.
        // This is a corrupted message.  Simply reset the buffers
        //
        OS_LOG_WARNING(FAC_NET, "StateQueueConnection::readMode "
                << "TERMINATED - "
                << " Message exceeds maximum buffer size. Lenth=" << len );
        abortRead();
        return;
      }
    }
    else
    {
      OS_LOG_INFO(FAC_NET,"StateQueueConnection::readMode "
                << "More bytes required to complete message.  "
                << " Read: " << bytes_transferred
                << " Required: " << _moreReadRequired
                << " Expected: " << _lastExpectedPacketSize
                << " Data: " << len);
    }
  }
  else
  {
    //
    // This is a corrupted message.  Simply reset the buffers
    //
    OS_LOG_WARNING(FAC_NET, "StateQueueConnection::readMore "
                << "TERMINATED - "
                << " Invalid protocol headers." );

    abortRead();
    return;
  }
}
#endif

void StateQueueConnection::abortRead()
{
  _messageBuffer = std::string();
  _lastExpectedPacketSize = 0;
  _moreReadRequired = 0;
  boost::system::error_code ignored_ec;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  _socket.close(ignored_ec);
  _agent.listener().destroyConnection(shared_from_this());
}

void StateQueueConnection::start()
{
  OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::start() INVOKED");

  if (_socket.is_open())
  {
    _freshRead = true;
    _socket.async_read_some(boost::asio::buffer(_buffer),
              boost::bind(&StateQueueConnection::handleRead, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
  }
}

void StateQueueConnection::stop()
{
  OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::stop() INVOKED");
  boost::system::error_code ignored_ec;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  _socket.close(ignored_ec);
}

void StateQueueConnection::onInactivityTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    OS_LOG_WARNING(FAC_NET, "StateQueueConnection::onInactivityTimeout "
                  << "No activity on this socket for too long." );
    boost::system::error_code ignored_ec;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    _socket.close(ignored_ec);
    _agent.listener().destroyConnection(shared_from_this());
  }
}

void StateQueueConnection::startInactivityTimer()
{
  boost::system::error_code ignored_ec;
  _pInactivityTimer->cancel(ignored_ec);
  _pInactivityTimer->expires_from_now(boost::posix_time::milliseconds(_agent.inactivityThreshold() * 1000));
  _pInactivityTimer->async_wait(boost::bind(&StateQueueConnection::onInactivityTimeout, this, boost::asio::placeholders::error));

  OS_LOG_DEBUG(FAC_NET, "StateQueueConnection::startInactivityTimer "
          << " Session inactivity timeout set at " << _agent.inactivityThreshold() << " seconds.");
}
