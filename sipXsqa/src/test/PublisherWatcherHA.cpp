//
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
//
//
// $$
////////////////////////////////////////////////////////////////////////

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <sipxunit/TestUtilities.h>
#include "SQAAgentUtil.h"

class PublisherWatcherHATest : public CppUnit::TestCase
{
  CPPUNIT_TEST_SUITE(PublisherWatcherHATest);
  //CPPUNIT_TEST(PublisherWatcherRegularBehaviorTest);
  //CPPUNIT_TEST(OneLocalPublisherTwoHAAgentsOneRemoteWatcherTest);
//  CPPUNIT_TEST(DealerWorkerRegularBehaviorTest);
//  CPPUNIT_TEST(OneDealerAgentAgentOneWorkerTest);
  CPPUNIT_TEST_SUITE_END();


public:
  struct SQAClientData
  {
        int _type;
        std::string _applicationId;
        std::string _serviceAddress;
        std::string _servicePort;
        std::string _zmqEventId;
        std::size_t _poolSize;
        int _readTimeout;
        int _writeTimeout;
        int _keepAliveTicks;

        SQAClientData(
            int type,
            const std::string& applicationId,
            const std::string& serviceAddress,
            const std::string& servicePort,
            const std::string& zmqEventId,
            std::size_t poolSize,
            int readTimeout,
            int writeTimeout,
            int keepAliveTicks): _type(type),
                _applicationId(applicationId),
                _serviceAddress(serviceAddress),
                _servicePort(servicePort),
                _zmqEventId(zmqEventId),
                _poolSize(poolSize),
                _readTimeout(readTimeout),
                _writeTimeout(writeTimeout),
                _keepAliveTicks(keepAliveTicks)

    {}
  };

  void setUp()
  {
    _util.setProgram("/usr/local/sipxecs-local-feature-branch/bin/sipxsqa");
  }

  void tearDown()
  {
    std::vector<SQAAgentData::Ptr>::iterator it;
    for (it = _agentsData.begin(); it != _agentsData.end(); it++)
    {
      SQAAgentData::Ptr data = *it;

      _util.stopSQAAgent(data);
    }

    _agentsData.clear();
  }

  void checkSQAClientCoreAfterStartup(StateQueueClient* client, StateQueueClient::SQAClientCore* core, SQAClientData cd, bool serviceIsUp)
  {
    CPPUNIT_ASSERT(core->_owner == client);
    CPPUNIT_ASSERT(core->_type == cd._type);
    CPPUNIT_ASSERT(core->_keepAliveTimer != NULL);
    CPPUNIT_ASSERT(core->_signinTimer != NULL);
    CPPUNIT_ASSERT(core->_poolSize == cd._poolSize);

    CPPUNIT_ASSERT(((int)core->_clientPool.size()) == cd._poolSize);
    CPPUNIT_ASSERT( std::string::npos != cd._serviceAddress.find(core->_serviceAddress));
    CPPUNIT_ASSERT(core->_servicePort == cd._servicePort);
    CPPUNIT_ASSERT(core->_terminate == false);
    if (!SQAUtil::isPublisher(cd._type))
    {
      CPPUNIT_ASSERT(core->_zmqContext != NULL);
      CPPUNIT_ASSERT(core->_zmqSocket != NULL);
      CPPUNIT_ASSERT(core->_pEventThread != NULL);
    }
    else
    {
      CPPUNIT_ASSERT(core->_zmqContext == NULL);
      CPPUNIT_ASSERT(core->_zmqSocket == NULL);
      CPPUNIT_ASSERT(core->_pEventThread == NULL);
    }

    CPPUNIT_ASSERT(core->_zmqEventId == client->_zmqEventId);
    CPPUNIT_ASSERT(core->_applicationId == cd._applicationId);
    CPPUNIT_ASSERT(core->_clientPointers.size() == cd._poolSize);
    CPPUNIT_ASSERT(core->_expires == 10);
    CPPUNIT_ASSERT(core->_subscriptionExpires == SQA_SIGNIN_INTERVAL_SEC);
    CPPUNIT_ASSERT(core->_backoffCount == 0);
    if (serviceIsUp)
    {
      CPPUNIT_ASSERT(core->_currentSigninTick == SQA_SIGNIN_INTERVAL_SEC * .75);
      CPPUNIT_ASSERT(core->_currentKeepAliveTicks == cd._keepAliveTicks);
      CPPUNIT_ASSERT(core->_isAlive == true);
      CPPUNIT_ASSERT(!core->_publisherAddress.empty());
    }
    else
    {
      CPPUNIT_ASSERT(core->_currentSigninTick == SQA_SIGNIN_ERROR_INTERVAL_SEC);
      CPPUNIT_ASSERT(core->_currentKeepAliveTicks == SQA_KEEP_ALIVE_ERROR_INTERVAL_SEC);
      CPPUNIT_ASSERT(core->_isAlive == false);
      CPPUNIT_ASSERT(core->_publisherAddress.empty());
    }
    CPPUNIT_ASSERT(!core->_localAddress.empty());
    CPPUNIT_ASSERT(core->_keepAliveTicks == cd._keepAliveTicks);


    CPPUNIT_ASSERT(core->_signinAttempts >= 1);
    CPPUNIT_ASSERT(core->_keepAliveAttempts >= 1);
    if (SQAUtil::isPublisher(core->_type))
    {
      CPPUNIT_ASSERT(core->_subscribeAttempts == 0);
      CPPUNIT_ASSERT(core->_subscribeState == SQAOpNotDone);
      if (serviceIsUp)
      {
        CPPUNIT_ASSERT(core->_signinState == SQAOpOK);
        CPPUNIT_ASSERT(core->_keepAliveState == SQAOpOK);
      }
      else
      {
        CPPUNIT_ASSERT(core->_signinState == SQAOpFailed);
        CPPUNIT_ASSERT(core->_keepAliveState == SQAOpFailed);
      }
    }
    else
    {
      if (serviceIsUp)
      {
        CPPUNIT_ASSERT(core->_subscribeAttempts >= 1);
        CPPUNIT_ASSERT(core->_signinState == SQAOpOK);
        CPPUNIT_ASSERT(core->_subscribeState == SQAOpOK);
        CPPUNIT_ASSERT(core->_keepAliveState == SQAOpOK);
      }
      else
      {
        CPPUNIT_ASSERT(core->_subscribeAttempts == 0);
        CPPUNIT_ASSERT(core->_signinState == SQAOpFailed);
        CPPUNIT_ASSERT(core->_subscribeState == SQAOpNotDone);
        CPPUNIT_ASSERT(core->_keepAliveState == SQAOpFailed);
      }
    }
  }

  void checkSQAClientCoreAfterStop(StateQueueClient::SQAClientCore* core)
  {
    CPPUNIT_ASSERT(core->_terminate == true);
    CPPUNIT_ASSERT(core->_zmqContext == 0);
    CPPUNIT_ASSERT(core->_zmqSocket == 0);
    CPPUNIT_ASSERT(core->_pEventThread == 0);
    CPPUNIT_ASSERT(core->_keepAliveTimer == 0);
    CPPUNIT_ASSERT(core->_signinTimer == 0);
  //  CPPUNIT_ASSERT(core->_pIoServiceThread == 0);

//    ClientPool _clientPool;
//    std::vector<BlockingTcpClient::Ptr> _clientPointers;
  }

  void checkStateQueueClientAfterStartup(StateQueueClient& client, SQAClientData cd, bool serviceIsUp)
  {
    CPPUNIT_ASSERT(client._type == cd._type);
    CPPUNIT_ASSERT(client._terminate == false);
    CPPUNIT_ASSERT(client._rawEventId == cd._zmqEventId);
    CPPUNIT_ASSERT(client._applicationId  == cd._applicationId);
    //TODO Any checks here? CPPUNIT_ASSERT(publisher._eventQueue  == 1000);
    CPPUNIT_ASSERT(client._expires  == 10);
    CPPUNIT_ASSERT(client._core);
    //TODO check this too std::string _zmqEventId;
  }

  void checkSQAClientAfterStop(StateQueueClient& client)
  {
    checkSQAClientCoreAfterStop(client._core);

    CPPUNIT_ASSERT(client._terminate == true);
  }

  void checkPublisherAfterStartup(
      StateQueueClient& publisher,
      SQAClientData cd,
      bool serviceIsUp)
  {
    checkStateQueueClientAfterStartup(publisher, cd, serviceIsUp);
    checkSQAClientCoreAfterStartup(&publisher, publisher._core, cd, serviceIsUp);
  }

  void checkDealerAfterStartup(
      StateQueueClient& publisher,
      SQAClientData cd,
      bool serviceIsUp)
  {
    checkPublisherAfterStartup(publisher, cd, serviceIsUp);
  }

  void checkWatcherAfterStartup(StateQueueClient& watcher,
      SQAClientData cd,
      bool serviceIsUp)

  {
    checkStateQueueClientAfterStartup(watcher, cd, serviceIsUp);
    checkSQAClientCoreAfterStartup(&watcher, watcher._core, cd, serviceIsUp);
  }

  void checkWorkerAfterStartup(StateQueueClient& worker,
      SQAClientData cd,
      bool serviceIsUp)

  {
    checkWatcherAfterStartup(worker, cd, serviceIsUp);
  }

  void checkMSCoreAfterStartup(StateQueueClient& worker, int coreIdx, SQAClientData cd, bool serviceIsUp)
  {
    if (coreIdx > 0)
    {
    std::stringstream strm;
    strm << cd._applicationId << "-" << coreIdx;
    cd._applicationId = strm.str();
    }

    CPPUNIT_ASSERT(worker._cores.size() > (unsigned int)coreIdx);

    checkSQAClientCoreAfterStartup(&worker, worker._cores[coreIdx], cd, serviceIsUp);
  }


  void checkRegularPublishWatch(StateQueueClient& publisher, StateQueueClient& watcher, const std::string& eventId, const std::string& data, bool noresponse)
  {
    std::string wEventId;
    std::string wEventData;


   // TEST: Regular no-response publish / watch should work
   CPPUNIT_ASSERT(publisher.publish(eventId, data, noresponse));
   CPPUNIT_ASSERT(watcher.watch(wEventId, wEventData, 50));

   std::string sqwEventId = "sqw." + eventId;
   // TEST: External publish uses directly the eventId as messageId with no modifications
   CPPUNIT_ASSERT(boost::starts_with(wEventId.c_str(), sqwEventId));
   // TEST: Verify that the eventId has the proper format with sqw.eventId.hex4-hex4
   CPPUNIT_ASSERT(SQAUtil::validateId(wEventId, SQAUtil::SQAClientPublisher, eventId));
   // TEST: Verify that received data match what was sent
   CPPUNIT_ASSERT(0 == wEventData.compare(data));
  }

  void checkRegularEnqueuePop(StateQueueClient& dealer, StateQueueClient& worker, const std::string& eventId, const std::string& data, int serviceId)
  {
    std::string wEventId;
    std::string wEventData;
    int wServiceId=0;


   // TEST: Regular no-response publish / watch should work
   CPPUNIT_ASSERT(dealer.enqueue(data));
   CPPUNIT_ASSERT(worker.pop(wEventId, wEventData, wServiceId, 50));

   std::string sqaEventId = "sqa." + eventId;
   // TEST: External publish uses directly the eventId as messageId with no modifications
   CPPUNIT_ASSERT(boost::starts_with(wEventId.c_str(), sqaEventId));
   // TEST: Verify that the eventId has the proper format with sqw.eventId.hex4-hex4
   CPPUNIT_ASSERT(SQAUtil::validateId(wEventId, SQAUtil::SQAClientDealer, eventId));
   // TEST: Verify that received data match what was sent
   CPPUNIT_ASSERT(0 == wEventData.compare(data));
   std::cout << serviceId << " " << wServiceId;
   CPPUNIT_ASSERT(serviceId == wServiceId);
   CPPUNIT_ASSERT(worker.erase(wEventId, wServiceId));
  }

  void PublisherWatcherRegularBehaviorTest()
  {
    //prepare one local sqa agent data (no ha)
    _agentsData.clear();
    _util.generateSQAAgentData(_agentsData, 1, true);

    // prepare publisher to local for events of type "reg"
    SQAClientData pd(SQAUtil::SQAClientPublisher, "Publisher",_agentsData[0]->sqaControlAddress,_agentsData[0]->sqaControlPort, "reg", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* publisher = new StateQueueClient(pd._type, pd._applicationId, pd._serviceAddress, pd._servicePort, pd._zmqEventId, pd._poolSize);

    // TEST: Publisher started but cannot connect to agent (agent is not started yet)
    checkPublisherAfterStartup(*publisher, pd, false);

    // TEST: Publish should fail
    CPPUNIT_ASSERT(!publisher->publish("reg", "reg-data", true));
    CPPUNIT_ASSERT(!publisher->publish("reg", "reg-data", false));

    // prepare watchers to local for events of type "reg"
    SQAClientData wd(SQAUtil::SQAClientWatcher, "Watcher", _agentsData[0]->sqaControlAddress, _agentsData[0]->sqaControlPort, "reg", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* watcher = new StateQueueClient(wd._type, wd._applicationId, wd._serviceAddress, wd._servicePort, wd._zmqEventId, wd._poolSize);

    // TEST: Watcher started but cannot connect to agent (agent is not started yet)
    checkWatcherAfterStartup(*watcher, wd, false);

    std::string eventId;
    std::string eventData;
    // TEST: Watch should timeout
    CPPUNIT_ASSERT(!watcher->watch(eventId, eventData, 50));

    //start agent
    CPPUNIT_ASSERT(_util.startSQAAgent(_agentsData[0]));

    // give time to publisher and watcher to connect
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    // TEST: Publisher connected to agent
    checkPublisherAfterStartup(*publisher, pd, true);
    // TEST: Watcher connected to agent
    checkWatcherAfterStartup(*watcher, wd, true);


    checkRegularPublishWatch(*publisher, *watcher, "reg", "reg.1.data", true);
    checkRegularPublishWatch(*publisher, *watcher, "reg", "reg.1.data", false);

    // take down agent
    CPPUNIT_ASSERT(_util.stopSQAAgent(_agentsData[0]));
    _agentsData.clear();
    // give time to lose connection
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    // TEST: Publish should fail
    CPPUNIT_ASSERT(!publisher->publish("reg", "reg-data", false));
    CPPUNIT_ASSERT(!publisher->publish("reg", "reg-data", true));
    // TEST: Watch should timeout
    CPPUNIT_ASSERT(!watcher->watch(eventId, eventData, 50));

    publisher->terminate();
    checkSQAClientAfterStop(*publisher);
    delete publisher;

    watcher->terminate();
    checkSQAClientAfterStop(*watcher);
    delete watcher;
  }

  void OneLocalPublisherTwoHAAgentsOneRemoteWatcherTest()
  {
    _agentsData.clear();
    _util.generateSQAAgentData(_agentsData, 2, true);

    CPPUNIT_ASSERT(_util.startSQAAgent(_agentsData[0]));
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CPPUNIT_ASSERT(_util.startSQAAgent(_agentsData[1]));
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    // prepare publisher to local and remote for events of type "reg"
    SQAClientData pd(SQAUtil::SQAClientPublisher, "PublisherLocal",_agentsData[0]->sqaControlAddress,_agentsData[0]->sqaControlPort, "reg", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* publisher1 = new StateQueueClient(pd._type, pd._applicationId, pd._serviceAddress, pd._servicePort, pd._zmqEventId, pd._poolSize);
    // prepare watchers to local and remote for events of type "reg"
    SQAClientData wd(SQAUtil::SQAClientWatcher, "WatcherRemote", _agentsData[1]->sqaControlAddress, _agentsData[1]->sqaControlPort, "reg", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* watcher2 = new StateQueueClient(wd._type, wd._applicationId, _agentsData[0]->sqaControlAddressAll, wd._servicePort, wd._zmqEventId, wd._poolSize);

    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    checkPublisherAfterStartup(*publisher1, pd, true);
    checkWatcherAfterStartup(*watcher2, wd, true);

   checkRegularPublishWatch(*publisher1, *watcher2, "reg", "reg.1.data", true);
   checkRegularPublishWatch(*publisher1, *watcher2, "reg", "reg.1.data", false);

    publisher1->terminate();
    checkSQAClientAfterStop(*publisher1);
    delete publisher1;

    watcher2->terminate();
    checkSQAClientAfterStop(*watcher2);
    delete watcher2;

    CPPUNIT_ASSERT(_util.stopSQAAgent(_agentsData[0]));
    CPPUNIT_ASSERT(_util.stopSQAAgent(_agentsData[1]));
    _agentsData.clear();
  }

  void DealerWorkerRegularBehaviorTest()
  {
    //prepare one local sqa agent data (no ha)
    _agentsData.clear();
    _util.generateSQAAgentData(_agentsData, 1, true);

    // prepare publisher to local for events of type "reg"
    SQAClientData dd(SQAUtil::SQAClientDealer, "Dealer",_agentsData[0]->sqaControlAddress,_agentsData[0]->sqaControlPort, "reg", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* dealer = new StateQueueClient(dd._type, dd._applicationId, dd._serviceAddress, dd._servicePort, dd._zmqEventId, dd._poolSize);

    // TEST: Publisher started but cannot connect to agent (agent is not started yet)
    checkPublisherAfterStartup(*dealer, dd, false);

    // TEST: Publish should fail
    CPPUNIT_ASSERT(!dealer->enqueue("reg-data"));

    // prepare watchers to local for events of type "reg"
    SQAClientData wd(SQAUtil::SQAClientWorker, "Worker", _agentsData[0]->sqaControlAddress, _agentsData[0]->sqaControlPort, "reg", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* worker = new StateQueueClient(wd._type, wd._applicationId, wd._serviceAddress, wd._servicePort, wd._zmqEventId, wd._poolSize);

    // TEST: Watcher started but cannot connect to agent (agent is not started yet)
    checkWatcherAfterStartup(*worker, wd, false);

    std::string eventId;
    std::string eventData;
    int serviceId=0;
    // TEST: Watch should timeout
    CPPUNIT_ASSERT(!worker->pop(eventId, eventData, serviceId, 50));

    //start agent
    CPPUNIT_ASSERT(_util.startSQAAgent(_agentsData[0]));

    // give time to publisher and watcher to connect
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    // TEST: Publisher connected to agent
    checkPublisherAfterStartup(*dealer, dd, true);
    // TEST: Watcher connected to agent
    checkWatcherAfterStartup(*worker, wd, true);

    checkRegularEnqueuePop(*dealer, *worker, "reg", "reg.1.data", 0);

    // take down agent
    CPPUNIT_ASSERT(_util.stopSQAAgent(_agentsData[0]));
    _agentsData.clear();
    // give time to lose connection
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    // TEST: Publish should fail
    CPPUNIT_ASSERT(!dealer->enqueue("reg-data"));
    // TEST: Watch should timeout
    CPPUNIT_ASSERT(!worker->pop(eventId, eventData, serviceId, 50));

    dealer->terminate();
    checkSQAClientAfterStop(*dealer);
    delete dealer;

    worker->terminate();
    checkSQAClientAfterStop(*worker);
    delete worker;
  }

  void OneDealerAgentAgentOneWorkerTest()
  {
    _agentsData.clear();
    _util.generateSQAAgentData(_agentsData, 2, true);

    CPPUNIT_ASSERT(_util.startSQAAgent(_agentsData[0]));
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CPPUNIT_ASSERT(_util.startSQAAgent(_agentsData[1]));
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    // prepare publisher to local for events of type "reg"
    SQAClientData dd(SQAUtil::SQAClientDealer, "DealerLocal",_agentsData[0]->sqaControlAddress,_agentsData[0]->sqaControlPort, "reg1", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* dealer1 = new StateQueueClient(dd._type, dd._applicationId, dd._serviceAddress, dd._servicePort, dd._zmqEventId, dd._poolSize);

    // prepare watchers to local for events of type "reg"
    SQAClientData dd2(SQAUtil::SQAClientDealer, "DealerRemote",_agentsData[1]->sqaControlAddress,_agentsData[1]->sqaControlPort, "reg2", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* dealer2 = new StateQueueClient(dd2._type, dd2._applicationId, dd2._serviceAddress, dd2._servicePort, dd2._zmqEventId, dd2._poolSize);
    SQAClientData wdLocal(SQAUtil::SQAClientWorker, "WorkerRemote", _agentsData[1]->sqaControlAddressAll, _agentsData[1]->sqaControlPort, "reg", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    SQAClientData wdRemote(SQAUtil::SQAClientWorker, "WorkerRemote", _agentsData[0]->sqaControlAddressAll, _agentsData[0]->sqaControlPort, "reg", 1, SQA_CONN_READ_TIMEOUT, SQA_CONN_WRITE_TIMEOUT, SQA_KEEP_ALIVE_INTERVAL_SEC);
    StateQueueClient* worker2 = new StateQueueClient(wdLocal._type, wdLocal._applicationId, _agentsData[0]->sqaControlAddressAll, wdLocal._servicePort, wdLocal._zmqEventId, wdLocal._poolSize);

    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    checkDealerAfterStartup(*dealer1, dd, true);
    checkDealerAfterStartup(*dealer2, dd2, true);

    checkWorkerAfterStartup(*worker2, wdLocal, true);

    //CPPUNIT_ASSERT(worker2->startMultiService(_agentsData[0]->sqaControlAddressAll, _agentsData[0]->sqaControlPortAll));
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    checkMSCoreAfterStartup(*worker2, 0, wdRemote, true);
    checkMSCoreAfterStartup(*worker2, 1, wdLocal, true);

    checkRegularEnqueuePop(*dealer2, *worker2, "reg2", "reg2.data", 1);
    checkRegularEnqueuePop(*dealer1, *worker2, "reg1", "reg1.data", 0);

    dealer1->terminate();
    checkSQAClientAfterStop(*dealer1);
    delete dealer1;

    dealer2->terminate();
    checkSQAClientAfterStop(*dealer2);
    delete dealer2;


    worker2->terminate();
    checkSQAClientAfterStop(*worker2);
    delete worker2;

    CPPUNIT_ASSERT(_util.stopSQAAgent(_agentsData[0]));
    CPPUNIT_ASSERT(_util.stopSQAAgent(_agentsData[1]));
    _agentsData.clear();
  }

  std::vector<SQAAgentData::Ptr> _agentsData;
  SQAAgentUtil _util;
};


CPPUNIT_TEST_SUITE_REGISTRATION(PublisherWatcherHATest);
