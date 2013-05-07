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

#include "sqa/StateQueueClient.h"

StateQueueClient::SQAClientCore::BlockingTcpClient::BlockingTcpClient(
        const std::string& applicationId,
        boost::asio::io_service& ioService,
        int readTimeout,
        int writeTimeout,
        short key) :
        _applicationId(applicationId),
        _ioService(ioService),
        _resolver(_ioService),
        _pSocket(0),
        _isConnected(false),
        _readTimeout(readTimeout),
        _writeTimeout(writeTimeout),
        _key(key),
        _readTimer(_ioService),
        _writeTimer(_ioService),
        _connectTimer(_ioService)
      {
      }

StateQueueClient::SQAClientCore::BlockingTcpClient::~BlockingTcpClient()
      {
          if (_pSocket)
          {
              delete _pSocket;
              _pSocket = 0;
          }
      }

void StateQueueClient::SQAClientCore::BlockingTcpClient::setReadTimeout(boost::asio::ip::tcp::socket& socket, int milliseconds)
      {
        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = milliseconds * 1000;
        setsockopt(socket.native(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::setWriteTimeout(boost::asio::ip::tcp::socket& socket, int milliseconds)
      {
        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = milliseconds * 1000;
        setsockopt(socket.native(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::startReadTimer()
      {
        boost::system::error_code ec;
        _readTimer.expires_from_now(boost::posix_time::milliseconds(_readTimeout), ec);
        _readTimer.async_wait(boost::bind(&BlockingTcpClient::onReadTimeout, this, boost::asio::placeholders::error));
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::startWriteTimer()
      {
        boost::system::error_code ec;
        _writeTimer.expires_from_now(boost::posix_time::milliseconds(_writeTimeout), ec);
        _writeTimer.async_wait(boost::bind(&BlockingTcpClient::onWriteTimeout, this, boost::asio::placeholders::error));
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::startConnectTimer()
      {
        boost::system::error_code ec;
        _connectTimer.expires_from_now(boost::posix_time::milliseconds(_readTimeout), ec);
        _connectTimer.async_wait(boost::bind(&BlockingTcpClient::onConnectTimeout, this, boost::asio::placeholders::error));
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::cancelReadTimer()
      {
        boost::system::error_code ec;
        _readTimer.cancel(ec);
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::cancelWriteTimer()
      {
        boost::system::error_code ec;
        _writeTimer.cancel(ec);
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::cancelConnectTimer()
      {
        boost::system::error_code ec;
        _connectTimer.cancel(ec);
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::onReadTimeout(const boost::system::error_code& e)
      {
        if (e)
          return;
        close();
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
            << " - " << _readTimeout << " milliseconds.");
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::onWriteTimeout(const boost::system::error_code& e)
      {
        if (e)
          return;
        close();
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
            <<" - " << _writeTimeout << " milliseconds.");
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::onConnectTimeout(const boost::system::error_code& e)
      {
        if (e)
          return;
        close();
        OS_LOG_ERROR(FAC_NET,  LOG_TAG_WID(_applicationId)
            << " - " << _readTimeout << " milliseconds.");
      }

      void StateQueueClient::SQAClientCore::BlockingTcpClient::close()
      {
        if (_pSocket)
        {
          boost::system::error_code ignored_ec;
         _pSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
         _pSocket->close(ignored_ec);
         _isConnected = false;
         OS_LOG_INFO(FAC_NET,  LOG_TAG_WID(_applicationId)
             << "  - socket deleted.");
        }
      }

      bool StateQueueClient::SQAClientCore::BlockingTcpClient::connect(const std::string& serviceAddress, const std::string& servicePort)
      {
        //
        // Close the previous socket;
        //
        close();

        if (_pSocket)
        {
          delete _pSocket;
          _pSocket = 0;
        }

        _pSocket = new boost::asio::ip::tcp::socket(_ioService);

        OS_LOG_INFO(FAC_NET,  LOG_TAG_WID(_applicationId)
            << " creating new connection to " << serviceAddress << ":" << servicePort);

        _serviceAddress = serviceAddress;
        _servicePort = servicePort;

        try
        {
          boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), serviceAddress.c_str(), servicePort.c_str());
          boost::asio::ip::tcp::resolver::iterator hosts = _resolver.resolve(query);

          ConnectTimer timer(this);
          //////////////////////////////////////////////////////////////////////////
          // Only works in 1.47 version of asio.  1.46 doesnt have this utility func
          // boost::asio::connect(*_pSocket, hosts);
          _pSocket->connect(hosts->endpoint()); // so we use the connect member
          //////////////////////////////////////////////////////////////////////////
          _isConnected = true;
          OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
              << " creating new connection to " << serviceAddress << ":" << servicePort << " SUCESSFUL.");
        }
        catch(std::exception e)
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << " failed with error " << e.what());
          _isConnected = false;
        }

        return _isConnected;
      }


      bool StateQueueClient::SQAClientCore::BlockingTcpClient::connect()
      {
        if(_serviceAddress.empty() || _servicePort.empty())
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << " remote address is not set");
          return false;
        }

        return connect(_serviceAddress, _servicePort);
      }

      bool StateQueueClient::SQAClientCore::BlockingTcpClient::send(const StateQueueMessage& request)
      {
        assert(_pSocket);
        std::string data = request.data();

        if (data.size() > SQA_CONN_MAX_READ_BUFF_SIZE - 1) /// Account for the terminating char "_"
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << " data size: " << data.size() << " maximum buffer length of " << SQA_CONN_MAX_READ_BUFF_SIZE - 1);
          return false;
        }

        short version = 1;
        unsigned long len = (unsigned long)data.size() + 1; /// Account for the terminating char "_"
        std::stringstream strm;
        strm.write((char*)(&version), sizeof(version));
        strm.write((char*)(&_key), sizeof(_key));
        strm.write((char*)(&len), sizeof(len));
        strm << data << "_";
        std::string packet = strm.str();
        boost::system::error_code ec;
        bool ok = false;

        {
          WriteTimer timer(this);
          ok = _pSocket->write_some(boost::asio::buffer(packet.c_str(), packet.size()), ec) > 0;
        }

        if (!ok || ec)
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << " write_some error: " << ec.message());
          _isConnected = false;
          return false;
        }
        return true;
      }

      bool StateQueueClient::SQAClientCore::BlockingTcpClient::receive(StateQueueMessage& response)
      {
        assert(_pSocket);
        unsigned long len = getNextReadSize();
        if (!len)
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << " next read size is empty.");
          return false;
        }

        char responseBuff[len];
        boost::system::error_code ec;
        {
          ReadTimer timer(this);
          _pSocket->read_some(boost::asio::buffer((char*)responseBuff, len), ec);
        }

        if (ec)
        {
          if (boost::asio::error::eof == ec)
          {
            OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
                << " remote closed the connection, read_some error: " << ec.message());
          }
          else
          {
            OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
                << " read_some error: " << ec.message());
          }

          _isConnected = false;
          return false;
        }
        std::string responseData(responseBuff, len);
        return response.parseData(responseData);
      }

      bool StateQueueClient::SQAClientCore::BlockingTcpClient::sendAndReceive(const StateQueueMessage& request, StateQueueMessage& response)
      {
        if (send(request))
          return receive(response);
        return false;
      }

      unsigned long StateQueueClient::SQAClientCore::BlockingTcpClient::getNextReadSize()
      {
        bool hasVersion = false;
        bool hasKey = false;
        unsigned long remoteLen = 0;
        while (!hasVersion || !hasKey)
        {
          short remoteVersion;
          short remoteKey;

          //TODO: Refactor the code below to do one read for the three fields
          //
          // Read the version (must be 1)
          //
          while (true)
          {

            boost::system::error_code ec;
            _pSocket->read_some(boost::asio::buffer((char*)&remoteVersion, sizeof(remoteVersion)), ec);
            if (ec)
            {
              if (boost::asio::error::eof == ec)
              {
                OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
                    << " remote closed the connection, read_some error: " << ec.message());
              }
              else
              {
                OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
                    << " Unable to read version "
                    << "ERROR: " << ec.message());
              }

              _isConnected = false;
              return 0;
            }
            else if (remoteVersion == 1)
               {
                 hasVersion = true;
                 break;
               }
          }

          while (true)
          {

            boost::system::error_code ec;
            _pSocket->read_some(boost::asio::buffer((char*)&remoteKey, sizeof(remoteKey)), ec);
            if (ec)
            {
              if (boost::asio::error::eof == ec)
              {
                OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
                    << " remote closed the connection, read_some error: " << ec.message());
              }
              else
              {
                OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
                    << " Unable to read secret key "
                    << "ERROR: " << ec.message());
              }

              _isConnected = false;
              return 0;
            }
            else
            {
              if (remoteKey >= SQA_KEY_MIN && remoteKey <= SQA_KEY_MAX)
              {
                hasKey = true;
                break;
              }
            }
          }
        }

        boost::system::error_code ec;
        _pSocket->read_some(boost::asio::buffer((char*)&remoteLen, sizeof(remoteLen)), ec);
        if (ec)
        {
          if (boost::asio::error::eof == ec)
          {
            OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
                << " remote closed the connection, read_some error: " << ec.message());
          }
          else
          {
            OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
                << " Unable to read secret packet length "
                << "ERROR: " << ec.message());
          }

          _isConnected = false;
          return 0;
        }

        return remoteLen;
      }

    void StateQueueClient::SQAClientCore::init(int readTimeout, int writeTimeout)
    {
      if (!SQAUtil::isPublisher(_type))
      {
        _zmqContext = new zmq::context_t(1);
        _zmqSocket = new zmq::socket_t(*_zmqContext,ZMQ_SUB);
        int linger = SQA_LINGER_TIME_MILLIS; // milliseconds
        _zmqSocket->setsockopt(ZMQ_LINGER, &linger, sizeof(int));
      }

      for (std::size_t i = 0; i < _poolSize; i++)
      {
        //TODO: check _ioService ptr
        BlockingTcpClient* pClient = new BlockingTcpClient(_applicationId, _ioService, readTimeout, writeTimeout, i == 0 ? SQA_KEY_ALPHA : SQA_KEY_DEFAULT );
        pClient->connect(_serviceAddress, _servicePort);

        if (_localAddress.empty())
          _localAddress = pClient->getLocalAddress();

        BlockingTcpClient::Ptr client(pClient);
        _clientPointers.push_back(client);
        _clientPool.enqueue(client);
      }

      OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
          << " zmqEventId: " <<  _zmqEventId << " CREATED");

      boost::system::error_code ignored(boost::system::errc::success, boost::system::system_category ());

      _signinTimer = new boost::asio::deadline_timer(_ioService, boost::posix_time::seconds(SQA_SIGNIN_ERROR_INTERVAL_SEC));
      signinLoop(ignored);

      _houseKeepingTimer = new boost::asio::deadline_timer(_ioService, boost::posix_time::seconds(SQA_KEEP_ALIVE_ERROR_INTERVAL_SEC));
      keepAliveLoop(ignored);

      _pIoServiceThread = new boost::thread(boost::bind(&boost::asio::io_service::run, &_ioService));


      if (!SQAUtil::isPublisher(_type))
      {
        _pEventThread = new boost::thread(boost::bind(&SQAClientCore::eventLoop, this));
      }
    }

    StateQueueClient::SQAClientCore::SQAClientCore(
          StateQueueClient* owner,
          int type,
          const std::string& applicationId,
          const std::string& serviceAddress,
          const std::string& servicePort,
          const std::string& zmqEventId,
          std::size_t poolSize,
          int readTimeout,
          int writeTimeout,
          int keepAliveTicks
          ) :
            _owner(owner),
            _type(type),
            _ioService(),
            _pIoServiceThread(0),
            _poolSize(poolSize),
            _clientPool(_poolSize),
            _serviceAddress(serviceAddress),
            _servicePort(servicePort),
            _terminate(false),
            _zmqContext(0),
            _zmqSocket(0),
            _pEventThread(0),
            _zmqEventId(zmqEventId),
            _applicationId(applicationId),
            _expires(10),
            _subscriptionExpires(SQA_SIGNIN_INTERVAL_SEC),
            _backoffCount(0),
            _currentSigninTick(-1),
            _keepAliveTicks(keepAliveTicks),
            _currentKeepAliveTicks(keepAliveTicks),
            _isAlive(true),
            _signinState(SQAOpNotDone),
            _signinAttempts(0),
            _subscribeState(SQAOpNotDone),
            _subscribeAttempts(0),
            _keepAliveState(SQAOpNotDone),
            _keepAliveAttempts(0)

    {
      init(readTimeout, writeTimeout);
    }

    void StateQueueClient::SQAClientCore::terminate()
    {
      if (_terminate)
        return;

      logout();

       _terminate = true;

       if (_zmqContext)
       {
         delete _zmqContext;
         _zmqContext = 0;
       }

      if (_zmqSocket)
      {
        delete _zmqSocket;
        _zmqSocket = 0;
      }

      OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
          << " waiting for event thread to exit.");
      if (_pEventThread)
      {
        _pEventThread->join();
        delete _pEventThread;
        _pEventThread = 0;
      }

      _ioService.stop();

      if (_houseKeepingTimer)
      {
        _houseKeepingTimer->cancel();
        delete _houseKeepingTimer;
        _houseKeepingTimer = 0;
      }

      if (_signinTimer)
      {
        _signinTimer->cancel();
        delete _signinTimer;
        _signinTimer = 0;
      }

      if (_pIoServiceThread)
      {
        _pIoServiceThread->join();
        delete _pIoServiceThread;
        _pIoServiceThread = 0;
      }

      OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
          << "Ok");
    }

    bool StateQueueClient::SQAClientCore::isConnected()
    {
      for (std::vector<BlockingTcpClient::Ptr>::const_iterator iter = _clientPointers.begin();
              iter != _clientPointers.end(); iter++)
      {
        BlockingTcpClient::Ptr pClient = *iter;
        if (pClient->isConnected())
          return true;
      }
      return false;
    }

    void StateQueueClient::SQAClientCore::keepAliveLoop(const boost::system::error_code& e)
    {
      if (!e && !_terminate)
      {
        //
        // send keep-alives
        //
        for (unsigned i = 0; i < _poolSize; i++)
        {
          StateQueueMessage ping;
          StateQueueMessage pong;
          ping.setType(StateQueueMessage::Ping);
          ping.set("message-app-id", _applicationId.c_str());

          OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
              << "send keep-alive ping to " << _serviceAddress << ":" << _servicePort);
          if (sendAndReceive(ping, pong))
          {
            if (pong.getType() == StateQueueMessage::Pong)
            {
              //
              // Reset it back to the default value
              //
              _currentKeepAliveTicks = _keepAliveTicks;
              _isAlive = true;
              _keepAliveState = SQAOpOK;

              OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
                  << " Keep-alive response received from " << _serviceAddress << ":" << _servicePort);
            }
          }
          else
          {
            //
            // Reset the keep-alive to 1 so we attempt to reconnect every second
            //
            _currentKeepAliveTicks = SQA_KEEP_ALIVE_ERROR_INTERVAL_SEC;
            _isAlive = false;
            _keepAliveState = SQAOpFailed;
            OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
                << " keep-alive failed to " << _serviceAddress << ":" << _servicePort);
          }
        }

        if (!_terminate)
        {
          _keepAliveAttempts++;
          OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
              << ", reschedule keep-alive in: " << _currentKeepAliveTicks
              << ", keep-alives so far: " << _keepAliveAttempts);
          _houseKeepingTimer->expires_from_now(boost::posix_time::seconds(_currentKeepAliveTicks));
          _houseKeepingTimer->async_wait(boost::bind(&StateQueueClient::SQAClientCore::keepAliveLoop, this, boost::asio::placeholders::error));
        }
      }
      else if (e)
      {
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
            << " keepAliveLoop timer failed with error: " <<  e.message());
      }
      else if (_terminate)
      {
        OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
            << " terminate requested, keepAliveLoop aborted");
      }
    }


    bool StateQueueClient::SQAClientCore::subscribe(const std::string& eventId, const std::string& sqaAddress)
    {
      assert(!SQAUtil::isPublisher(_type));

      OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
          << " eventId=" << eventId << " address=" << sqaAddress);

      _subscribeAttempts++;

      try
      {
        _zmqSocket->connect(sqaAddress.c_str());
        _zmqSocket->setsockopt(ZMQ_SUBSCRIBE, eventId.c_str(), eventId.size());

      }catch(std::exception e)
      {
        OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
            << " eventId=" << eventId << " address=" << sqaAddress << " FAILED!  Error: " << e.what());
        _subscribeState = SQAOpFailed;
        return false;
      }

      _subscribeState = SQAOpOK;
      return true;
    }

    void StateQueueClient::SQAClientCore::signinLoop(const boost::system::error_code& e)
    {
      if (!e && !_terminate)
      {
        StateQueueMessage request(StateQueueMessage::Signin, _type);
        request.set("message-app-id", _applicationId.c_str());
        request.set("subscription-expires", _subscriptionExpires);
        request.set("subscription-event", _zmqEventId.c_str());

        std::string clientType = SQAUtil::getServiceTypeStr(_type);
        request.set("service-type", clientType);

        if (SQAUtil::isExternal(_type))
        {
          request.set("external-service", true);
        }

        OS_LOG_NOTICE(FAC_NET, LOG_TAG_WID(_applicationId)
            << " Type=" << clientType << " SIGNIN");


        bool ok = false;
        StateQueueMessage response;
        if (sendAndReceive(request, response))
        {
          ok = response.get("message-data", _publisherAddress);
        }
        else
        {
          ok = false;
        }

        OS_LOG_NOTICE(FAC_NET, LOG_TAG_WID(_applicationId)
            << " Type=" << clientType << " SQA=" << _publisherAddress << ((ok) ? " SUCCEEDED" : " FAILED"));


        if (ok)
        {
          _signinState = SQAOpOK;
          _currentSigninTick = _subscriptionExpires * .75;
        }
        else
        {
          _signinState = SQAOpFailed;
          _currentSigninTick = SQA_SIGNIN_ERROR_INTERVAL_SEC;
        }

        if (!_terminate)
        {
          _signinAttempts++;
          OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
              << ", reschedule signin in: " << _currentSigninTick
              << ", signin so far: " << _signinAttempts);

          _signinTimer->expires_from_now(boost::posix_time::seconds(_currentSigninTick));
          _signinTimer->async_wait(boost::bind(&StateQueueClient::SQAClientCore::signinLoop, this, boost::asio::placeholders::error));
        }
      }
      else if (e)
      {
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
            << " signinLoop timer failed with error: " <<  e.message());
      }
      else if (_terminate)
      {
        OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
            << " terminate requested, signinLoop aborted");
      }
    }

    bool StateQueueClient::SQAClientCore::logout()
    {
      StateQueueMessage request(StateQueueMessage::Logout, _type);

      request.set("message-app-id", _applicationId.c_str());
      request.set("subscription-event", _zmqEventId.c_str());

      request.set("service-type", SQAUtil::getServiceTypeStr(_type));

      StateQueueMessage response;
      return sendAndReceive(request, response);
    }

    void StateQueueClient::SQAClientCore::eventLoop()
    {
      const int retryTime = SQA_SIGNIN_ERROR_INTERVAL_SEC * 500;
      while(!_terminate)
      {
        if (SQAOpOK == _signinState)
        {
          break;
        }
        else
        {
          OS_LOG_WARNING(FAC_NET, LOG_TAG_WID(_applicationId)
              << " Network Queue did no respond.  Retrying SIGN IN after " << retryTime << " ms.");
          boost::this_thread::sleep(boost::posix_time::milliseconds(retryTime));
        }
      }

      bool firstHit = true;
      if (!_terminate)
      {
        while (!_terminate)
        {
          if (subscribe(_zmqEventId, _publisherAddress))
          {
            break;
          }
          else
          {
            OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
                << " Is unable to SUBSCRIBE to SQA service @ " << _publisherAddress);
            boost::this_thread::sleep(boost::posix_time::milliseconds(retryTime));
          }
        }

        assert(!SQAUtil::isPublisher(_type));

        while (!_terminate)
        {
          std::string id;
          std::string data;
          int count = 0;
          if (readEvent(id, data, count))
          {
            if (_terminate)
              break;

            OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
                << "received event: " << id);
            OS_LOG_DEBUG(FAC_NET, "SQAClientCore::eventLoop"
                << " this:" << this << " [" << _applicationId << "]"
                << " received data: " << data);

            if (SQAUtil::isWorker(_type))
            {
              OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
                  << "popping data: " << id);
              do_pop(firstHit, count, id, data);
            }else if (SQAUtil::isWatcherOnly(_type))
            {
              OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
                  << " watching data: " << id);
              do_watch(firstHit, count, id, data);
            }
          }
          else
          {
            if (_terminate)
            {
              break;
            }
          }
          firstHit = false;
        }
      }

      if (SQAUtil::isWatcherOnly(_type))
      {
        do_watch(firstHit, 0, SQA_TERMINATE_STRING, SQA_TERMINATE_STRING);
      }
      else if (SQAUtil::isWorker(_type))
      {
        do_pop(firstHit, 0, SQA_TERMINATE_STRING, SQA_TERMINATE_STRING);
      }

      _zmqSocket->close();

      OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
          << " TERMINATED.");
    }

    void StateQueueClient::SQAClientCore::do_pop(bool firstHit, int count, const std::string& id, const std::string& data)
    {
      //
      // Check if we are the last succesful popper.
      // If count >= 2 we will skip the next message
      // after we have successfully popped
      //

      if (id.substr(0, 3) == "sqw")
      {
        OS_LOG_WARNING(FAC_NET, LOG_TAG_WID(_applicationId)
            << " do_pop dropping event " << id);
        return;
      }

      if (id == "__TERMINATE__")
      {
        StateQueueMessage terminate;
        terminate.setType(StateQueueMessage::Pop);
        terminate.set("message-id", "__TERMINATE__");
        terminate.set("message-data", "__TERMINATE__");
        _owner->getEventQueue()->enqueue(terminate.data());
        return;
      }

      if (!firstHit && count >= 2 && _backoffCount < count - 1 )
      {
        _backoffCount++; //  this will ensure that we participate next time
        boost::this_thread::yield();
      }
      //
      // Check if we are in the exclude list
      //
      if (data.find(_applicationId.c_str()) != std::string::npos)
      {
        //
        // We are still considered the last popper so don't toggle?
        //
        _backoffCount++;
        OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
            << " do_pop is not allowed to pop " << id);
        return;
      }
      //
      // Pop it
      //
      StateQueueMessage pop;
      pop.setType(StateQueueMessage::Pop);
      pop.set("message-id", id.c_str());
      pop.set("message-app-id", _applicationId.c_str());
      pop.set("message-expires", _expires);

      OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
          << " Popping event " << id);

      StateQueueMessage popResponse;
      if (!sendAndReceive(pop, popResponse))
      {
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
            << " do_pop unable to send pop command for event " << id);
        _backoffCount++;
        return;
      }

      //
      // Check if Pop is successful
      //
      std::string messageResponse;
      popResponse.get("message-response", messageResponse);
      if (messageResponse != "ok")
      {
        std::string messageResponseError;
        popResponse.get("message-error", messageResponseError);
        OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
                << " Dropping event " << id
                << " Error: " << messageResponseError);
        _backoffCount++;
      }
      else
      {
        std::string messageId;
        popResponse.get("message-id", messageId);
        std::string messageData;
        popResponse.get("message-data", messageData);
        OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
                << " Popped event " << messageId << " -- " << messageData);
        _owner->getEventQueue()->enqueue(popResponse.data());
        _backoffCount = 0;
      }
    }

    void StateQueueClient::SQAClientCore::do_watch(bool firstHit, int count, const std::string& id, const std::string& data)
    {
      OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
          << "Received watcher data " << id);
      StateQueueMessage watcherData;
      watcherData.set("message-id", id);
      watcherData.set("message-data", data);
      _owner->getEventQueue()->enqueue(watcherData.data());
    }

    bool StateQueueClient::SQAClientCore::readEvent(std::string& id, std::string& data, int& count)
    {
      assert(!SQAUtil::isPublisher(_type));

      try
      {
        if (!zmq_receive(_zmqSocket, id))
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << "0mq failed to receive ID segment.");
          return false;
        }

        std::string address;
        if (!zmq_receive(_zmqSocket, address))
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << "0mq failed to receive ADDR segment.");
          return false;
        }

        //
        // Read the data vector
        //
        if (!zmq_receive(_zmqSocket, data))
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << "0mq failed to receive DATA segment.");
          return false;
        }

        //
        // Read number of subscribers active
        //
        std::string strcount;
        if (!zmq_receive(_zmqSocket, strcount))
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << "0mq failed to receive COUNT segment.");
          return false;
        }

          count = boost::lexical_cast<int>(strcount);
      }
      catch(std::exception e)
      {
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
            << " Unknown exception: " << e.what());
        return false;
      }
      return true;
    }

     void StateQueueClient::SQAClientCore::zmq_free (void *data, void *hint)
    {
        free (data);
    }
    //  Convert string to 0MQ string and send to socket
    bool StateQueueClient::SQAClientCore::zmq_send (zmq::socket_t & socket, const std::string & data)
    {
      char * buff = (char*)malloc(data.size());
      memcpy(buff, data.c_str(), data.size());
      zmq::message_t message((void*)buff, data.size(), zmq_free, 0);
      bool rc = socket.send(message);
      return (rc);
    }

    //  Sends string as 0MQ string, as multipart non-terminal
    bool StateQueueClient::SQAClientCore::zmq_sendmore (zmq::socket_t & socket, const std::string & data)
    {
      char * buff = (char*)malloc(data.size());
      memcpy(buff, data.c_str(), data.size());
      zmq::message_t message((void*)buff, data.size(), zmq_free, 0);
      bool rc = socket.send(message, ZMQ_SNDMORE);
      return (rc);
    }

     bool StateQueueClient::SQAClientCore::zmq_receive (zmq::socket_t *socket, std::string& value)
    {
        zmq::message_t message;
        socket->recv(&message);
        if (!message.size())
          return false;
        value = std::string(static_cast<char*>(message.data()), message.size());
        return true;
    }

    bool StateQueueClient::SQAClientCore::sendNoResponse(const StateQueueMessage& request)
    {
      if (!_isAlive)
        return false;

      BlockingTcpClient::Ptr conn;
      if (!_clientPool.dequeue(conn))
        return false;

      if (!conn->isConnected() && !conn->connect(_serviceAddress, _servicePort))
      {
        //
        // Put it back to the queue.  The server is down.
        //
        _clientPool.enqueue(conn);
        return false;
      }

      bool sent = conn->send(request);
      _clientPool.enqueue(conn);
      return sent;
    }

    bool StateQueueClient::SQAClientCore::sendAndReceive(const StateQueueMessage& request, StateQueueMessage& response)
    {

      if (!_isAlive && request.getType() != StateQueueMessage::Ping)
      {
        //
        // Only allow ping requests to get through when connection is not alive
        //
        return false;
      }

      BlockingTcpClient::Ptr conn;
      if (!_clientPool.dequeue(conn))
      {
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
            << " Unable to retrieve a TCP connection for pool.");
        return false;
      }

      if (!conn->isConnected() && !conn->connect(_serviceAddress, _servicePort))
      {
        //
        // Put it back to the queue.  The server is down.
        //
        _clientPool.enqueue(conn);
        return false;
      }

      bool sent = conn->sendAndReceive(request, response);
      _clientPool.enqueue(conn);
      return sent;
    }

    void StateQueueClient::getControlAddressPort(std::string& address, std::string& port)
    {
      std::ostringstream sqaconfig;
      sqaconfig << SIPX_CONFDIR << "/" << "sipxsqa-client.ini";
      ServiceOptions configOptions(sqaconfig.str());
      if (configOptions.parseOptions())
      {
        bool enabled = false;
        if (configOptions.getOption("enabled", enabled, enabled) && enabled)
        {
          configOptions.getOption("sqa-control-address", address);
          configOptions.getOption("sqa-control-port", port);
        }
        else
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << " Unable to read connection information from " << sqaconfig.str());
          assert(false);
        }
      }
    }

    void StateQueueClient::getMSControlAddressPort(std::string& addresses, std::string& ports)
    {
      std::ostringstream sqaconfig;
      sqaconfig << SIPX_CONFDIR << "/" << "sipxsqa-client.ini";
      ServiceOptions configOptions(sqaconfig.str());
      if (configOptions.parseOptions())
      {
        bool enabled = false;
        if (configOptions.getOption("enabled", enabled, enabled) && enabled)
        {
          configOptions.getOption("sqa-control-addresses", addresses);
          configOptions.getOption("sqa-control-ports", ports);
        }
        else
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << " Unable to read connection information from " << sqaconfig.str());
          assert(false);
        }
      }
    }


    StateQueueClient::StateQueueClient(
        int type,
        const std::string& applicationId,
        const std::string& serviceAddress,
        const std::string& servicePort,
        const std::string& zmqEventId,
        std::size_t poolSize,
        int readTimeout,
        int writeTimeout,
        int keepAliveTicks
        ) :
    _type(type),
    _applicationId(applicationId),
    _serviceAddress(serviceAddress),
    _servicePort(servicePort),
    _terminate(false),
    _rawEventId(zmqEventId),
    _poolSize(poolSize),
    _readTimeout(readTimeout),
    _writeTimeout(writeTimeout),
    _keepAliveTicks(keepAliveTicks),
    _eventQueue(1000),
    _expires(10),
    _core(0)
  {
    if (SQAUtil::isWatcherOnly(_type))
      _zmqEventId = "sqw.";
    else
      _zmqEventId = "sqa.";

    _zmqEventId += zmqEventId;

    _core = new SQAClientCore(this, _type, _applicationId, _serviceAddress,
        _servicePort, _zmqEventId, _poolSize, _readTimeout, _writeTimeout, _keepAliveTicks);
    _cores.push_back(_core);

    OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
        << " for zmqEventId '" <<  _zmqEventId << "' CREATED");
  }

    StateQueueClient::StateQueueClient(
        int type,
        const std::string& applicationId,
        const std::string& zmqEventId,
        std::size_t poolSize,
        int readTimeout,
        int writeTimeout,
        int keepAliveTicks
        ) :
    _type(type),
    _applicationId(applicationId),
    _terminate(false),
    _rawEventId(zmqEventId),
    _poolSize(poolSize),
    _readTimeout(readTimeout),
    _writeTimeout(writeTimeout),
    _keepAliveTicks(keepAliveTicks),
    _eventQueue(1000),
    _expires(10),
    _core(0)
  {
    if (SQAUtil::isWatcherOnly(_type))
      _zmqEventId = "sqw.";
    else
      _zmqEventId = "sqa.";

    _zmqEventId += zmqEventId;

    getControlAddressPort(_serviceAddress, _servicePort);

    std::stringstream strm;
    strm << _applicationId << "-" << _cores.size();
    std::string coreApplicationId = strm.str();

    _core = new SQAClientCore(this, _type, coreApplicationId, _serviceAddress,
        _servicePort, _zmqEventId, _poolSize, _readTimeout, _writeTimeout, _keepAliveTicks);
    _cores.push_back(_core);

    OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
        <<  " for zmqEventId " << _zmqEventId << " CREATED");
  }

    StateQueueClient::~StateQueueClient()
  {
    terminate();
  }

    bool StateQueueClient::startMultiService(const std::string& servicesAddresses,
        const std::string& servicesPorts)
    {
      if (!SQAUtil::isMulti(_type))
      {
        OS_LOG_WARNING(FAC_NET, LOG_TAG_WID(_applicationId)
                << " cannot start multi service for this type");
        return false;
      }

      std::vector<std::string> addresses;
      std::vector<std::string> ports;
      boost::algorithm::split(addresses, servicesAddresses, boost::is_any_of(","), boost::token_compress_on);
      boost::algorithm::split(ports, servicesPorts, boost::is_any_of(","), boost::token_compress_on);

      if (addresses.size() != ports.size())
      {
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
                << " for worker multiservice cannot match addresses with ports");
        return false;
      }
      else if (0 == addresses.size())
      {
        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
                << " for worker multiservice could not find any addresses");
        return false;
      }

      for (unsigned int i = 0; i < addresses.size(); i++)
      {
        if (addresses[i] == _serviceAddress && ports[i] == _servicePort)
        {
          continue;
        }

        std::stringstream strm;
        strm << _applicationId << "-" << _cores.size();
        std::string coreApplicationId = strm.str();

        SQAClientCore* core = new SQAClientCore(this, _type, coreApplicationId, addresses[i],
            ports[i], _zmqEventId, _poolSize, _readTimeout, _writeTimeout, _keepAliveTicks);

        _cores.push_back(core);
      }

      _core = _cores[0];
      return true;
    }

    bool StateQueueClient::startMultiService()
    {
      std::string servicesAddresses;
      std::string servicesPorts;

      getMSControlAddressPort(servicesAddresses, servicesPorts);

      return startMultiService(servicesAddresses, servicesPorts);
    }


  void StateQueueClient::terminate()
  {
    if (_terminate)
      return;

     _terminate = true;

     std::vector<SQAClientCore*>::iterator it;
     for(it = _cores.begin(); it != _cores.end(); it++)
     {
       SQAClientCore* core = *it;

       core->terminate();
       delete core;
     }
     _cores.clear();

    OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
        << "Ok");
  }


  bool StateQueueClient::checkMessageResponse(StateQueueMessage& response)
  {
    std::string empty;
    return checkMessageResponse(response, empty);
  }

  bool StateQueueClient::checkMessageResponse(StateQueueMessage& response, const std::string& dataId)
  {
      std::string messageResponse;
      response.get("message-response", messageResponse);
      if (messageResponse == "ok")
      {
        OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
            << "Operation successful");
        return true;
      }
      else
      {

        std::string messageType;
        response.get("message-type", messageType);

        std::string messageResponseError;
        response.get("message-error", messageResponseError);

        if (dataId.empty())
        {
        std::string id;
        response.get("message-id", id);

        OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
            << "Operation '" << messageType << "' failed for id:" << id
            << ". Error: " << messageResponseError);

        }
        else
        {
          OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
              << "Operation '" << messageType << "' failed for dataId:" << dataId
              << ". Error: " << messageResponseError);
        }

        return false;
      }
  }


  bool StateQueueClient::pop(StateQueueMessage& ev)
  {
    std::string data;
    if (_eventQueue.dequeue(data))
    {
      ev.parseData(data);
      return true;
    }
    return false;
  }

  bool StateQueueClient::pop(StateQueueMessage& ev, int milliseconds)
  {
    std::string data;
    if (_eventQueue.dequeue(data, milliseconds))
    {
      ev.parseData(data);
      return true;
    }
    return false;
  }

  bool StateQueueClient::enqueue(const std::string& eventId, const std::string& data, int expires, bool publish)
  {
    //
    // Enqueue it
    //
    std::string messageTypeStr;
    StateQueueMessage::Type messageType = StateQueueMessage::Unknown;
    if (!publish)
    {
        messageType = StateQueueMessage::Enqueue;
        messageTypeStr = "Enqueue";
    }
    else
    {
        messageType = StateQueueMessage::EnqueueAndPublish;
        messageTypeStr = "EnqueueAndPublish";
    }

    StateQueueMessage enqueueRequest(messageType, _type, eventId);
    enqueueRequest.set("message-app-id", _applicationId.c_str());
    enqueueRequest.set("message-expires", expires);
    enqueueRequest.set("message-data", data);

    std::string id;
    enqueueRequest.get("message-id", id);
    OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
            << " operation: " << messageTypeStr
            << " message-id: "      << id
            << " message-app-id: "  << _applicationId
            << " message-data: "    << data
            << " message-expires: "    << expires);

    StateQueueMessage enqueueResponse;
    if (!_core->sendAndReceive(enqueueRequest, enqueueResponse))
    {
      OS_LOG_ERROR(FAC_NET, LOG_TAG_WID(_applicationId)
          << " FAILED");
      return false;
    }

    //
    // Check if Queue is successful
    //
    return checkMessageResponse(enqueueResponse);
  }

    bool StateQueueClient::internal_publish(const std::string& eventId, const std::string& data, bool noresponse)
    {
      StateQueueMessage enqueueRequest(StateQueueMessage::Publish);

      std::string messageId;
      if (!SQAUtil::isExternal(_type))
      {
          // For regular publishing message id will be generated from eventId.
          SQAUtil::generateId(messageId, _type, eventId);
      }
      else
      {
          // For external publishing message id will be eventId.
          messageId = eventId;
          enqueueRequest.set("message-no-external", true);
      }
      enqueueRequest.set("message-id", messageId.c_str());


      enqueueRequest.set("message-app-id", _applicationId.c_str());
      enqueueRequest.set("message-data", data);

      OS_LOG_DEBUG(FAC_NET,LOG_TAG_WID(_applicationId)
              << " message-id: "      << messageId
              << " message-app-id: "  << _applicationId
              << " message-data: "    << data);

      if (noresponse)
      {
        enqueueRequest.set("noresponse", noresponse);
        return _core->sendNoResponse(enqueueRequest);
      }
      else
      {
        StateQueueMessage enqueueResponse;
        if (!_core->sendAndReceive(enqueueRequest, enqueueResponse))
            return false;

        return checkMessageResponse(enqueueResponse);
      }
    }


  bool StateQueueClient::pop(std::string& id, std::string& data)
  {
    StateQueueMessage message;
    if (!pop(message))
      return false;
    return message.get("message-id", id) && message.get("message-data", data);
  }

  bool StateQueueClient::pop(std::string& id, std::string& data, int milliseconds)
  {
    StateQueueMessage message;
    if (!pop(message, milliseconds))
      return false;
    return message.get("message-id", id) && message.get("message-data", data);
  }


  bool StateQueueClient::watch(std::string& id, std::string& data)
  {
    OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
        << "(" << id << ") INVOKED");
    StateQueueMessage message;
    if (!pop(message))
      return false;

    return message.get("message-id", id) && message.get("message-data", data);
  }

  bool StateQueueClient::watch(std::string& id, std::string& data, int milliseconds)
  {
    OS_LOG_INFO(FAC_NET, LOG_TAG_WID(_applicationId)
        << "(" << id << ") INVOKED" );
    StateQueueMessage message;
    if (!pop(message, milliseconds))
      return false;

    return message.get("message-id", id) && message.get("message-data", data);
  }


  bool StateQueueClient::enqueue(const std::string& data, int expires, bool publish)
  {
    if (!SQAUtil::isDealer(_type))
      return false;

    return enqueue(_rawEventId, data, expires, publish);
  }


  bool StateQueueClient::publish(const std::string& eventId, const std::string& data, bool noresponse)
  {
    if (!SQAUtil::isPublisher(_type))
      return false;

    if (eventId.empty())
        return false;

    return internal_publish(eventId, data, noresponse);
  }

  bool StateQueueClient::publish(const std::string& eventId, const char* data, int dataLength, bool noresponse)
  {
    std::string buff = std::string(data, dataLength);
    return publish(eventId, data, dataLength);
  }

  bool StateQueueClient::publishAndPersist(int workspace, const std::string& eventId, const std::string& data, int expires)
  {
    if (!SQAUtil::isPublisher(_type))
      return false;

    StateQueueMessage request(StateQueueMessage::PublishAndPersist, _type, eventId);

    std::string id;
    request.get("message-id", id);

    request.set("message-app-id", _applicationId.c_str());
    request.set("message-data-id", id.c_str());
    request.set("message-data", data);
    request.set("message-expires", expires);
    request.set("workspace", workspace);

    OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
        << " message-id: "      << id
        << " message-app-id: "  << _applicationId
        << " message-data-id: " << id
        << " message-data: "    << data
        << " message-expires: " << expires
        << " workspace: "       << workspace);

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    //
    // Check if Queue is successful
    //
    return checkMessageResponse(response);
  }


  bool StateQueueClient::erase(const std::string& id)
  {
    StateQueueMessage request(StateQueueMessage::Erase, _type);
    request.set("message-app-id", _applicationId.c_str());

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    return checkMessageResponse(response);
  }

  bool StateQueueClient::persist(int workspace, const std::string& dataId, int expires)
  {
    StateQueueMessage request(StateQueueMessage::Persist, _type);
    request.set("message-app-id", _applicationId.c_str());
    request.set("message-expires", expires);
    request.set("workspace", workspace);
    request.set("message-data-id", dataId.c_str());

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    return checkMessageResponse(response, dataId);
  }

  bool StateQueueClient::set(int workspace, const std::string& dataId, const std::string& data, int expires)
  {
    StateQueueMessage request(StateQueueMessage::Set, _type);
    request.set("message-app-id", _applicationId.c_str());
    request.set("message-expires", expires);
    request.set("workspace", workspace);
    request.set("message-data-id", dataId.c_str());
    request.set("message-data", data);

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    return checkMessageResponse(response, dataId);
  }

  bool StateQueueClient::mset(int workspace, const std::string& mapId, const std::string& dataId, const std::string& data, int expires)
  {
    StateQueueMessage request(StateQueueMessage::MapSet, _type);
    request.set("message-app-id", _applicationId.c_str());
    request.set("message-expires", expires);
    request.set("workspace", workspace);
    request.set("message-map-id", mapId.c_str());
    request.set("message-data-id", dataId.c_str());
    request.set("message-data", data);

    std::string id;
    request.get("message-id", id);

    OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
        << " message-id: "      << id
        << " message-app-id: "  << _applicationId
        << " workspace: "    << workspace
        << " message-map-id: "    << mapId
        << " message-data-id: " << dataId
        << " message-data: "    << data
        << " message-expires: "    << expires);

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    return checkMessageResponse(response, dataId);
  }

  bool StateQueueClient::get(int workspace, const std::string& dataId, std::string& data)
  {
    StateQueueMessage request(StateQueueMessage::Get, _type);
    request.set("message-app-id", _applicationId.c_str());
    request.set("workspace", workspace);
    request.set("message-data-id", dataId.c_str());

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    return checkMessageResponse(response, dataId);
  }

  bool StateQueueClient::mget(int workspace, const std::string& mapId, const std::string& dataId, std::string& data)
  {
    StateQueueMessage request(StateQueueMessage::MapGet, _type);
    request.set("message-app-id", _applicationId.c_str());
    request.set("workspace", workspace);
    request.set("message-map-id", mapId.c_str());
    request.set("message-data-id", dataId.c_str());

    std::string id;
    request.get("message-id", id);

    OS_LOG_DEBUG(FAC_NET, LOG_TAG_WID(_applicationId)
        << " message-id: "      << id
        << " message-app-id: "  << _applicationId
        << " workspace: "    << workspace
        << " message-map-id: "    << mapId
        << " message-data-id: " << id);

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    return checkMessageResponse(response, dataId);
  }

  bool StateQueueClient::mgetm(int workspace, const std::string& mapId, std::map<std::string, std::string>& smap)
  {
    StateQueueMessage request(StateQueueMessage::MapGetMultiple, _type);
    request.set("message-app-id", _applicationId.c_str());
    request.set("workspace", workspace);
    request.set("message-map-id", mapId.c_str());

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    bool ret = checkMessageResponse(response, mapId);
    if (true == ret)
    {
      std::string data;
      response.get("message-data", data);

      StateQueueMessage message;
      if (!message.parseData(data))
        return false;

      return message.getMap(smap);
    }

    return ret;
  }

  bool StateQueueClient::mgeti(int workspace, const std::string& mapId, const std::string& dataId, std::string& data)
  {
    StateQueueMessage request(StateQueueMessage::MapGetInc, _type);
    request.set("message-app-id", _applicationId.c_str());
    request.set("workspace", workspace);
    request.set("message-map-id", mapId.c_str());
    request.set("message-data-id", dataId.c_str());

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    bool ret = checkMessageResponse(response, dataId);
    if (true == ret)
    {
      return response.get("message-data", data);
    }

    return ret;
  }

  bool StateQueueClient::remove(int workspace, const std::string& dataId)
  {
    StateQueueMessage request(StateQueueMessage::Remove, _type);
    request.set("message-app-id", _applicationId.c_str());
    request.set("message-data-id", dataId.c_str());
    request.set("workspace", workspace);

    StateQueueMessage response;
    if (!_core->sendAndReceive(request, response))
      return false;

    return checkMessageResponse(response, dataId);
  }
