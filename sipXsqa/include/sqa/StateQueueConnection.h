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

#ifndef STATEQUEUECONNECTION_H
#define	STATEQUEUECONNECTION_H

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include "sqa/StateQueueClient.h"

class StateQueueAgent;


class StateQueueConnection : public boost::enable_shared_from_this<StateQueueConnection>, boost::noncopyable
{
public:
  typedef boost::asio::ip::tcp::socket::endpoint_type EndPoint;
  typedef boost::shared_ptr<StateQueueConnection> Ptr;
  struct Packet
  {
    short version; // Expecting version 1
    short size; // size of the data buffer
    char* data;
  };

  explicit StateQueueConnection(
      boost::asio::io_service& ioService,
      StateQueueAgent& agent);

  ~StateQueueConnection();

  void start();
  void stop();

  void handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);

  bool write(const std::string& data);
  boost::asio::ip::tcp::socket& socket();

  const std::string& getLocalAddress() const;
  unsigned short getLocalPort() const;
  const std::string& getRemoteAddress() const;
  unsigned short getRemotePort() const;
  const std::string& getApplicationId() const;
  void setApplicationId(const std::string& id);
  bool isAlphaConnection() const;
  void setCreationPublished();
  void abortRead();
  void initLocalAddressPort();
  bool isCreationPublished() const;

  void markExternalConnection();
  /// This connection was made from an external SQA agent

  bool isExternalConnection() const;
  /// Return true if this connection was made from an external SQA agent

protected:
  void readMore(std::size_t bytes_transferred);
  void startInactivityTimer();
  void onInactivityTimeout(const boost::system::error_code&);
  boost::asio::io_service& _ioService;
  StateQueueAgent& _agent;
  boost::asio::ip::tcp::socket _socket;
  boost::asio::ip::tcp::resolver _resolver;
  boost::array<char, SQA_CONN_MAX_READ_BUFF_SIZE> _buffer;

  std::string _messageBuffer;
  std::string _spillOverBuffer;
  std::size_t _moreReadRequired;
  std::size_t _lastExpectedPacketSize;
  std::string _localAddress;
  std::string _remoteAddress;
  unsigned short _localPort;
  unsigned short _remotePort;
  boost::asio::deadline_timer* _pInactivityTimer;
  std::string _applicationId;
  bool _isAlphaConnection;  /// true if this connection is used for ping-pong
  bool _isCreationPublished;
  bool _isExternalConnection; /// true if this connection remote endpoint is an external SQA Agent
  bool _freshRead;
};


//
// Inline
//

inline boost::asio::ip::tcp::socket& StateQueueConnection::socket()
{
  return _socket;
}

inline const std::string& StateQueueConnection::getLocalAddress() const
{
  return _localAddress;
}

inline unsigned short StateQueueConnection::getLocalPort() const
{
  return _localPort;
}

inline const std::string& StateQueueConnection::getRemoteAddress() const
{
  return _remoteAddress;
}

inline unsigned short StateQueueConnection::getRemotePort() const
{
  return _remotePort;
}

inline const std::string& StateQueueConnection::getApplicationId() const
{
  return _applicationId;
}

inline void StateQueueConnection::setApplicationId(const std::string& id)
{
  _applicationId = id;
}

inline bool StateQueueConnection::isAlphaConnection() const
{
  return _isAlphaConnection;
}

inline void StateQueueConnection::setCreationPublished()
{
  _isCreationPublished = true;
}

inline  bool StateQueueConnection::isCreationPublished() const
{
  return _isCreationPublished;
}

inline void StateQueueConnection::markExternalConnection()
{
    _isExternalConnection = true;
}

inline  bool StateQueueConnection::isExternalConnection() const
{
    return _isExternalConnection;
}

#endif	/* STATEQUEUECONNECTION_H */

