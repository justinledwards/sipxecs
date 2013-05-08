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

#ifndef StateQueueClient_H
#define	StateQueueClient_H

#include <cassert>
#include <zmq.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "StateQueueMessage.h"
#include "BlockingQueue.h"
#include <os/OsLogger.h>
#include <boost/lexical_cast.hpp>
#include "ServiceOptions.h"
#include "SQADefines.h"

#define SQA_LINGER_TIME_MILLIS 5000
#define SQA_TERMINATE_STRING "__TERMINATE__"
#define SQA_CONN_MAX_READ_BUFF_SIZE 65536
#define SQA_CONN_READ_TIMEOUT 1000
#define SQA_CONN_WRITE_TIMEOUT 1000
#define SQA_KEY_MIN 22172	//TODO: This define and its value needs to be documented
#define SQA_KEY_ALPHA 22180	//TODO: This define and its value needs to be documented
#define SQA_KEY_DEFAULT SQA_KEY_MIN
#define SQA_KEY_MAX 22200	//TODO: This define and its value needs to be documented
#define SQA_KEEP_ALIVE_INTERVAL_SEC 30 //sec
#define SQA_KEEP_ALIVE_ERROR_INTERVAL_SEC 1 //sec
#define SQA_SIGNIN_INTERVAL_SEC 1800 //sec
#define SQA_SIGNIN_ERROR_INTERVAL_SEC 1 //sec

class StateQueueClient : public boost::enable_shared_from_this<StateQueueClient>, private boost::noncopyable
{
public:
  class SQAClientCore : public boost::enable_shared_from_this<SQAClientCore>, private boost::noncopyable
  {
  public:

    class BlockingTcpClient
    {
    public:
      typedef boost::shared_ptr<BlockingTcpClient> Ptr;

      class ConnectTimer
      {
      public:
        ConnectTimer(BlockingTcpClient* pOwner) :
          _pOwner(pOwner)
        {
          _pOwner->startConnectTimer();
        }

        ~ConnectTimer()
        {
          _pOwner->cancelConnectTimer();
        }
        BlockingTcpClient* _pOwner;
      };

      class ReadTimer
      {
      public:
        ReadTimer(BlockingTcpClient* pOwner) :
          _pOwner(pOwner)
        {
          _pOwner->startReadTimer();
        }

        ~ReadTimer()
        {
          _pOwner->cancelReadTimer();
        }
        BlockingTcpClient* _pOwner;
      };

      class WriteTimer
      {
      public:
        WriteTimer(BlockingTcpClient* pOwner) :
          _pOwner(pOwner)
        {
          _pOwner->startWriteTimer();
        }

        ~WriteTimer()
        {
          _pOwner->cancelWriteTimer();
        }
        BlockingTcpClient* _pOwner;
      };

      BlockingTcpClient(
          const std::string& applicationId,
        boost::asio::io_service& ioService,
        int readTimeout = SQA_CONN_READ_TIMEOUT,
        int writeTimeout = SQA_CONN_WRITE_TIMEOUT,
        short key = SQA_KEY_DEFAULT);
      ~BlockingTcpClient();

      inline const char* getClassName() {return "BlockingTcpClient";}
      void setReadTimeout(boost::asio::ip::tcp::socket& socket, int milliseconds);

      void setWriteTimeout(boost::asio::ip::tcp::socket& socket, int milliseconds);
      void startReadTimer();

      void startWriteTimer();

      void startConnectTimer();

      void cancelReadTimer();
      void cancelWriteTimer();
      void cancelConnectTimer();
      void onReadTimeout(const boost::system::error_code& e);

      void onWriteTimeout(const boost::system::error_code& e);
      void onConnectTimeout(const boost::system::error_code& e);
      void close();
      bool connect();
      bool connect(const std::string& serviceAddress, const std::string& servicePort);

      bool send(const StateQueueMessage& request);
      bool receive(StateQueueMessage& response);

      bool sendAndReceive(const StateQueueMessage& request, StateQueueMessage& response);

      unsigned long getNextReadSize();

      bool isConnected() const
      {
        return _isConnected;
      }

      std::string getLocalAddress()
      {
        try
        {
          if (!_pSocket)
            return "";
          return _pSocket->local_endpoint().address().to_string();
        }
        catch(...)
        {
          return "";
        }
      }
    private:
      std::string _applicationId;
      boost::asio::io_service& _ioService;
      boost::asio::ip::tcp::resolver _resolver;
      boost::asio::ip::tcp::socket *_pSocket;
      std::string _serviceAddress;
      std::string _servicePort;
      bool _isConnected;
      int _readTimeout;
      int _writeTimeout;
      short _key;
      boost::asio::deadline_timer _readTimer;
      boost::asio::deadline_timer _writeTimer;
      boost::asio::deadline_timer _connectTimer;
      friend class SQAClientCore;
    };

  protected:
    StateQueueClient *_owner;
    int _idx;
    int _type;
    //boost::asio::io_service _ioService;
    //boost::thread* _pIoServiceThread;
    boost::asio::deadline_timer *_keepAliveTimer;
    boost::asio::deadline_timer *_signinTimer;
    std::size_t _poolSize;
    typedef BlockingQueue<BlockingTcpClient::Ptr> ClientPool;
    ClientPool _clientPool;
    std::string _serviceAddress;
    std::string _servicePort;
    bool _terminate;
    zmq::context_t* _zmqContext;
    zmq::socket_t* _zmqSocket;
    boost::thread* _pEventThread;
    std::string _zmqEventId;
    std::string _applicationId;
    std::vector<BlockingTcpClient::Ptr> _clientPointers;
    int _expires;
    int _subscriptionExpires;
    int _backoffCount;
    int _currentSigninTick;
    std::string _localAddress;
    int _keepAliveTicks;
    int _currentKeepAliveTicks;
    int _isAlive;

    enum SQAOpState _signinState;
    int  _signinAttempts;
    enum SQAOpState _subscribeState;
    bool _subscribeAttempts;
    enum SQAOpState _keepAliveState;
    int  _keepAliveAttempts;

    std::string _publisherAddress;

  public:
    void init(int readTimeout, int writeTimeout);

  public:
    SQAClientCore(
          StateQueueClient* owner,
          int idx,
          int type,
          const std::string& applicationId,
          const std::string& serviceAddress,
          const std::string& servicePort,
          const std::string& zmqEventId,
          std::size_t poolSize,
          int readTimeout = SQA_CONN_READ_TIMEOUT,
          int writeTimeout = SQA_CONN_WRITE_TIMEOUT,
          int keepAliveTicks = SQA_KEEP_ALIVE_INTERVAL_SEC
          ) ;

    ~SQAClientCore()
    {
      terminate();
    }

    inline const char* getClassName() {return "SQAClientCore";}

    const std::string& getLocalAddress()
    {
      return _localAddress;
    }

    void terminate();

    void setExpires(int expires)
    {
      _expires = expires;
    }

    bool isConnected();
    void keepAliveLoop(const boost::system::error_code& e);
  public:
    bool subscribe(const std::string& eventId, const std::string& sqaAddress);

    void signinLoop(const boost::system::error_code& e);

    bool logout();

    void eventLoop();

    void do_pop(bool firstHit, int count, const std::string& id, const std::string& data);
    void do_watch(bool firstHit, int count, const std::string& id, const std::string& data);
    bool readEvent(std::string& id, std::string& data, int& count);

    static void zmq_free (void *data, void *hint);
    //  Convert string to 0MQ string and send to socket
    static bool zmq_send (zmq::socket_t & socket, const std::string & data);
    //  Sends string as 0MQ string, as multipart non-terminal
    static bool zmq_sendmore (zmq::socket_t & socket, const std::string & data);
    static bool zmq_receive (zmq::socket_t *socket, std::string& value);

    bool sendNoResponse(const StateQueueMessage& request);

    bool sendAndReceive(const StateQueueMessage& request, StateQueueMessage& response);
    friend class PublisherWatcherHATest;
    friend class StateQueueClient;
  };

protected:
  int _type;
  std::string _applicationId;
  std::string _serviceAddress;
  std::string _servicePort;
  bool _terminate;
  std::string _zmqEventId;
  std::string _rawEventId;
  std::size_t _poolSize;
  int _readTimeout;
  int _writeTimeout;
  int _keepAliveTicks;
  typedef BlockingQueue<std::string> EventQueue;
  EventQueue _eventQueue;
  int _expires;
  SQAClientCore* _core;
  std::vector<SQAClientCore*> _cores;
  std::vector<SQAClientCore*> _fallbackCores;
  boost::asio::io_service _ioService;
  boost::thread* _pIoServiceThread;
  boost::asio::deadline_timer *_fallbackTimer;

  friend class PublisherWatcherHATest;

public:
  typedef SQAClientCore* ServiceId;
  StateQueueClient(
        int type,
        const std::string& applicationId,
        const std::string& serviceAddress,
        const std::string& servicePort,
        const std::string& zmqEventId,
        std::size_t poolSize,
        int readTimeout = SQA_CONN_READ_TIMEOUT,
        int writeTimeout = SQA_CONN_WRITE_TIMEOUT,
        int keepAliveTicks = SQA_KEEP_ALIVE_INTERVAL_SEC
        ) ;

  StateQueueClient(
        int type,
        const std::string& applicationId,
        const std::string& zmqEventId,
        std::size_t poolSize,
        int readTimeout = SQA_CONN_READ_TIMEOUT,
        int writeTimeout = SQA_CONN_WRITE_TIMEOUT,
        int keepAliveTicks = SQA_KEEP_ALIVE_INTERVAL_SEC
        ) ;

  ~StateQueueClient();

  boost::asio::io_service* getIoService() {return &_ioService;}

  void getControlAddressPort(std::string& address, std::string& port);
  void getMSControlAddressPort(std::string& addresses, std::string& ports);

  bool startMultiService();
  bool startMultiService(const std::string& servicesAddresses, const std::string& servicesPorts);
  bool setFallbackService();
  bool setFallbackService(int innactivityTimeout, const std::string& servicesAddresses, const std::string& servicesPorts);

  inline const char* getClassName() {return "StateQueueClient";}
  inline EventQueue* getEventQueue()  {return &_eventQueue;}

  const std::string& getLocalAddress()
  {
    return _core->getLocalAddress();
  }

  void terminate();
  void setExpires(int expires) { _core->setExpires(expires); }

  bool isConnected()
  {
    return _core->isConnected();
  }

  bool checkMessageResponse(StateQueueMessage& response);

  bool checkMessageResponse(StateQueueMessage& response, const std::string& dataId);
private:
  bool pop(StateQueueMessage& ev);
  bool pop(StateQueueMessage& ev, int milliseconds);

  bool enqueue(const std::string& eventId, const std::string& data, int expires = 30, bool publish= false);

    bool internal_publish(const std::string& eventId, const std::string& data, bool noresponse = false);

public:
  
  bool pop(std::string& id, std::string& data, int& serviceId);
  bool pop(std::string& id, std::string& data, int& serviceId, int milliseconds);
  bool watch(std::string& id, std::string& data);
  bool watch(std::string& id, std::string& data, int milliseconds);

  bool enqueue(const std::string& data, int expires = 30, bool publish = false);


  bool publish(const std::string& eventId, const std::string& data, bool noresponse);
  bool publish(const std::string& eventId, const char* data, int dataLength, bool noresponse);
  bool publishAndSet(int workspace, const std::string& eventId, const std::string& data, int expires);


  bool erase(const std::string& id, int serviceId=0);
  bool persist(int workspace, const std::string& dataId, int expires);
  bool set(int workspace, const std::string& dataId, const std::string& data, int expires);

  bool mset(int workspace, const std::string& mapId, const std::string& dataId, const std::string& data, int expires);

  bool get(int workspace, const std::string& dataId, std::string& data);

  bool mget(int workspace, const std::string& mapId, const std::string& dataId, std::string& data);

  bool mgetm(int workspace, const std::string& mapId, std::map<std::string, std::string>& smap);


  bool mgeti(int workspace, const std::string& mapId, const std::string& dataId, std::string& data);

  bool remove(int workspace, const std::string& dataId);
};


#endif	/* StateQueueClient_H */

