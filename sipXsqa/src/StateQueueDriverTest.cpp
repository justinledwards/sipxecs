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

#include "sqa/StateQueueDriverTest.h"
#include <iostream>
#include "sqa/UnitTest.h"

const unsigned int g_defaultSqaControlPort = 6240;
const unsigned int g_defaultZmqSubscriptionPort = 6242;
const unsigned int g_portIncrement = 100;

StateQueueDriverTest *g_driver = 0;

void terminateWatcherFunc(StateQueueClient *watcher)
{
    watcher->terminate();
}

void timedTerminateWatcher(int timeoutMsec, StateQueueClient *watcher)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(timeoutMsec));

    boost::thread *thread = new boost::thread(boost::bind(&terminateWatcherFunc, watcher));
    if (thread)
    {
        thread->join();
        delete thread;
        thread = NULL;
    }
}

//
// DEFINE_UNIT_TEST - Define a Test Group.  Must be called prior to DEFINE_TEST
// DEFINE_TEST - Define a new unit test belonging to a defined group
// DEFINE_RESOURCE - Register a resource that is accessible to unit tests in the same group
// GET_RESOURCE - Get the value of the resource that was previously created by DEFINE_RESOURCE
// ASSERT_COND(cond) - Assert if the logical condition is false
// ASSERT_STR_EQ(var1, var2) - Assert that two strings are  equal
// ASSERT_STR_CASELESS_EQ(var1, var2) - Assert that two strings are equal but ignoring case comparison
// ASSERT_STR_NEQ(var1, var2) - Asserts that two strings are not eual
// ASSERT_EQ(var1, var2) - Asserts that the two values are equal
// ASSERT_NEQ(var1, var2) - Asserts that the the values are not equal
// ASSERT_LT(var1, var2) - Asserts that the value var1 is less than value of var2
// ASSERT_GT(var1, var2)  Asserts that the value var1 is greater than value of var2
//

DEFINE_UNIT_TEST(TestDriver);

DEFINE_TEST(TestDriver, TestSimplePop)
{
  StateQueueClient* pClient = GET_RESOURCE(TestDriver, StateQueueClient*, "simple_pop_client");
  StateQueueClient* pPublisher = GET_RESOURCE(TestDriver, StateQueueClient*, "simple_publisher");
  pPublisher->enqueue("Hello SQA!");
  std::string messageId;
  std::string messageData;
  int serviceId;
  ASSERT_COND(pClient->pop(messageId, messageData, serviceId));
  ASSERT_STR_EQ(messageData, "Hello SQA!");
  ASSERT_COND(pClient->erase(messageId));
}

DEFINE_TEST(TestDriver, TestMultiplePop)
{
  StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");
  std::string address;
  std::string port;
  std::string publisher;
  _pAgent->options().getOption("sqa-control-address", address);
  _pAgent->options().getOption("sqa-control-port", port);
  _pAgent->options().getOption("zmq-subscription-address", publisher);
  StateQueueClient* pPublisher = GET_RESOURCE(TestDriver, StateQueueClient*, "simple_publisher");
  //
  // Create three threaded clients
  //
  ThreadedPop client1("StateQueueDriverTest-C1", address, port, "reg");
  ThreadedPop client2("StateQueueDriverTest-C2", address, port, "reg");
  ThreadedPop client3("StateQueueDriverTest-C3", address, port, "reg");

  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
  
  client1.start();
  client2.start();
  client3.start();

  int currentMax = 1500;
  for (int x = 0; x < 5; x++)
  {
    for (int i = 0; i < 500; i++)
    {
      pPublisher->enqueue("test multiple poppers-1", 10);
      pPublisher->enqueue("test multiple poppers-2", 10);
      pPublisher->enqueue("test multiple poppers-3", 10);
    }

    while(client1.total + client2.total + client3.total < currentMax)
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }

    std::cout << std::endl << "Iteration " << x << std::endl;
    std::cout << "Client 1 processed " << client1.total << " events." << std::endl;
    std::cout << "Client 2 processed " << client2.total << " events." << std::endl;
    std::cout << "Client 3 processed " << client3.total << " events." << std::endl;
    
    currentMax += 1500;
  }
}

DEFINE_TEST(TestDriver, TestGetSetErase)
{
  StateQueueClient* pClient = GET_RESOURCE(TestDriver, StateQueueClient*, "simple_pop_client");
  ASSERT_COND(pClient->set(1, "sample-set-data-id", "sample-set-data", 10));

  std::string sampleData;
  ASSERT_COND(pClient->get(1, "sample-set-data-id", sampleData));
  ASSERT_STR_EQ(sampleData, "sample-set-data");
  ASSERT_COND(pClient->remove(1, "sample-set-data-id"));
  ASSERT_COND(!pClient->get(1, "sample-set-data-id", sampleData));
}

DEFINE_TEST(TestDriver, TestSimplePersistGetErase)
{
  StateQueueClient* pClient = GET_RESOURCE(TestDriver, StateQueueClient*, "simple_pop_client");
  StateQueueClient* pPublisher = GET_RESOURCE(TestDriver, StateQueueClient*, "simple_publisher");
  ASSERT_COND(pPublisher->enqueue("Hello SQA!"));

  std::string eventId;
  std::string eventData;
  int serviceId;
  ASSERT_COND(pClient->pop(eventId, eventData, serviceId));
  ASSERT_COND(pClient->persist(1, eventId, 10));
  std::string sampleData;
  ASSERT_COND(pClient->get(1, eventId, sampleData));
  ASSERT_STR_EQ(sampleData, eventData);
  ASSERT_COND(pClient->remove(1, eventId));
  ASSERT_COND(!pClient->get(1, eventId, sampleData));
}

DEFINE_TEST(TestDriver, TestWatcher)
{
  StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");

  std::string address;
  std::string port;
  _pAgent->options().getOption("sqa-control-address", address);
  _pAgent->options().getOption("sqa-control-port", port);

  StateQueueClient* pPublisher = GET_RESOURCE(TestDriver, StateQueueClient*, "simple_publisher");
  StateQueueClient watcher(SQAUtil::SQAClientWatcher, "StateQueueDriverTest", address, port, "watcher-data", 1);

  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  ASSERT_COND(pPublisher->publish("watcher-data-sample", "Hello SQA!", false));
  std::string watcherData;
  std::string eventId;
  ASSERT_COND(watcher.watch(eventId, watcherData));
  ASSERT_STR_EQ(watcherData, "Hello SQA!");
}

DEFINE_TEST(TestDriver, TestPublishAndSet)
{
  StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");
  std::string address;
  std::string port;
  _pAgent->options().getOption("sqa-control-address", address);
  _pAgent->options().getOption("sqa-control-port", port);

  SQAPublisher publisher("TestPublishAndSet", address.c_str(), port.c_str(), 1, 100, 100);
  SQAWatcher watcher("TestPublishAndSet", address.c_str(), port.c_str(), "pub&persist", 1, 100, 100);
  boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  ASSERT_COND(publisher.publishAndSet(5, "pub&persist", "test-data", 10));
  SQAEvent* pEvent = watcher.watch();
  ASSERT_COND(pEvent);
  ASSERT_STR_EQ(pEvent->data, "test-data");
  
  char* data = watcher.get(5, pEvent->id);
  ASSERT_COND(data);
  ASSERT_STR_EQ(data, "test-data");
  free(data);
  delete pEvent;
}

DEFINE_TEST(TestDriver, TestDealAndPublish)
{
  StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");
  std::string address;
  std::string port;
  _pAgent->options().getOption("sqa-control-address", address);
  _pAgent->options().getOption("sqa-control-port", port);
  /*
   inline SQADealer::SQADealer(
  const char* applicationId, // Unique application ID that will identify this watcher to SQA
  const char* serviceAddress, // The IP address of the SQA
  const char* servicePort, // The port where SQA is listening for connections
  const char* eventId, // Event ID of the event being watched. Example: "sqa.not"
  int poolSize // Number of active connections to SQA
)
   */
  SQADealer dealer("TestDealAndPublish", address.c_str(), port.c_str(), "not", 1, 100, 100);
  SQAWatcher watcher("TestDealAndPublish", address.c_str(), port.c_str(), "not", 1, 100, 100);
  SQAWorker worker("TestDealAndPublish", address.c_str(), port.c_str(), "not", 1, 100, 100);
  boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  ASSERT_COND(dealer.dealAndPublish("test-data", 20));
  SQAEventEx* pEventEx = worker.fetchTask();
  ASSERT_COND(pEventEx);
  ASSERT_STR_EQ(pEventEx->data, "test-data");
  delete pEventEx;
  SQAEvent* pEvent = watcher.watch();
  ASSERT_COND(pEvent);
  ASSERT_STR_EQ(pEvent->data, "test-data");
  delete pEvent;
}

DEFINE_TEST(TestDriver, TestTimedMap)
{
  TimedMap set;
  std::string item1Value = "item-1";
  std::string item2Value = "item-2";
  set.insert("my-set-id", "item-1", item1Value, 1);
  set.insert("my-set-id", "item-2", item2Value, 1);
  boost::any item1, item2;
  ASSERT_COND(set.getItem("my-set-id", "item-1", item1));
  ASSERT_COND(set.getItem("my-set-id", "item-2", item2));
  ASSERT_STR_EQ(boost::any_cast<std::string&>(item1).c_str(),  item1Value.c_str());
  ASSERT_STR_EQ(boost::any_cast<std::string&>(item2).c_str(),  item2Value.c_str());
  //
  // Test getting all items
  //
  TimedMap::Items items;
  ASSERT_COND(set.getItems("my-set-id", items));
  ASSERT_STR_EQ(boost::any_cast<std::string&>(items["item-1"]).c_str(), item1Value.c_str());
  ASSERT_STR_EQ(boost::any_cast<std::string&>(items["item-2"]).c_str(), item2Value.c_str());
  //
  // Wait two seconds for items to expire.
  //
  boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
  set.cleanup();
  ASSERT_COND(!set.getItem("my-set-id", "item-1", item1));
  ASSERT_COND(!set.getItem("my-set-id", "item-2", item2));
}

DEFINE_TEST(TestDriver, TestMapGetSet)
{
  StateQueueClient* pClient = GET_RESOURCE(TestDriver, StateQueueClient*, "simple_pop_client");
  ASSERT_COND(pClient->mset(1, "sample-set-data-id", "cseq", "1", 10));

  std::string sampleData;
  ASSERT_COND(pClient->mget(1, "sample-set-data-id", "cseq", sampleData));
  ASSERT_STR_EQ(sampleData, "1");
  ASSERT_COND(pClient->mgeti(1, "sample-set-data-id", "cseq", sampleData));
  ASSERT_STR_EQ(sampleData, "2");
  ASSERT_COND(pClient->mgeti(1, "sample-set-data-id", "cseq", sampleData));
  ASSERT_STR_EQ(sampleData, "3");
}

DEFINE_TEST(TestDriver, TestMapGetSetPlugin)
{
  StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");
  std::string address;
  std::string port;
  _pAgent->options().getOption("sqa-control-address", address);
  _pAgent->options().getOption("sqa-control-port", port);
  SQAWatcher watcher("TestMapGetSetPlugin", address.c_str(), port.c_str(), "dummy", 1, 100, 100);
  watcher.mset(10, "TestMapGetSetPlugin", "cseq", "0", 10);
  char* cseq = watcher.mget(10, "TestMapGetSetPlugin", "cseq");
  ASSERT_STR_EQ(cseq, "0");
  free(cseq);
  int incremented = -1;
  ASSERT_COND(watcher.mgeti(10, "TestMapGetSetPlugin", "cseq", incremented));
  ASSERT_COND(incremented == 1);
  cseq = watcher.mget(10, "TestMapGetSetPlugin", "cseq");
  ASSERT_STR_EQ(cseq, "1");
  free(cseq);
  watcher.mset(10, "TestMapGetSetPlugin", "call-id", "test-call-id", 10);
  std::map<std::string, std::string> smap = watcher.mgetAll(10,"TestMapGetSetPlugin");
  ASSERT_COND(smap.find("cseq") != smap.end());
  ASSERT_COND(smap.find("call-id") != smap.end());
  ASSERT_STR_EQ(smap.find("cseq")->second.c_str(), "1");
  ASSERT_STR_EQ(smap.find("call-id")->second.c_str(), "test-call-id");
}

//***********************Publisher Tests*****************************************

// Test behavior of publish() when there is no connection to StateQueueAgent
DEFINE_TEST(TestDriver, TestPublishNoConnection)
{
    StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");

    std::string address;
    std::string port;
    // get the right address but use a wrong port
    _pAgent->options().getOption("sqa-control-address", address);
    port="60000";

    StateQueueClient* publisher = new StateQueueClient(SQAUtil::SQAClientPublisher, "TestPublisherPublishNoConnection", address, port, "reg", 2);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    //TEST: publish() should fail
    ASSERT_COND(!publisher->publish("no-conn-event-id", "no-conn-event-data", false));
    ASSERT_COND(!publisher->publish("no-conn-event-id", "no-conn-event-data", true));

    delete publisher;
}

// Test behavior of publish() when used by other types of services
DEFINE_TEST(TestDriver, TestPublishRestrictionToNonPublishers)
{
    StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");

    std::string address;
    std::string port;
    _pAgent->options().getOption("sqa-control-address", address);
    _pAgent->options().getOption("sqa-control-port", port);

    //TEST: Worker is not allowed to do publish()
    StateQueueClient* publisher = new StateQueueClient(SQAUtil::SQAClientWorker, "TestPublishRestrictionToNonPublishers", address, port, "reg", 2);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    ASSERT_COND(!publisher->publish("dummy-event-id", "dummy-event-data", false));
    delete publisher;

    //TEST: Watcher is not allowed to publish()
    publisher = new StateQueueClient(SQAUtil::SQAClientWatcher, "TestPublishRestrictionToNonPublishers", address, port, "reg", 2);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    ASSERT_COND(!publisher->publish("dummy-event-id", "dummy-event-data", false));
    delete publisher;

    //TODO: Do something with Dealer too
}

// Test regular behavior of publish()
DEFINE_TEST(TestDriver, TestPublishRegularBehavior)
{
    StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");

    std::string address;
    std::string port;
    _pAgent->options().getOption("sqa-control-address", address);
    _pAgent->options().getOption("sqa-control-port", port);

    // Prepare a publisher and a watcher
    StateQueueClient* publisher = new StateQueueClient(SQAUtil::SQAClientPublisher, "TestPublishRegularBehavior", address, port, "reg", 2);
    StateQueueClient watcher(SQAUtil::SQAClientWatcher, "StateQueueDriverTest", address, port, "reg",  1);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    std::string eventId;
    std::string eventData;

    // TEST: Regular no-response publish / watch should work
    ASSERT_COND(publisher->publish("reg", "regular-event-data", false));
    ASSERT_COND(watcher.watch(eventId, eventData));
    ASSERT_COND(SQAUtil::validateId(eventId, SQAUtil::SQAClientPublisher, "reg"));
    //TODO: Verify that the eventId has the proper format with sqw.eventId.hex4-hex4
    ASSERT_STR_EQ(eventData, "regular-event-data");

    // TEST: Regular with response publish / watch should work
    ASSERT_COND(publisher->publish("reg", "regular-event-data", true));
    ASSERT_COND(watcher.watch(eventId, eventData));
    ASSERT_COND(SQAUtil::validateId(eventId, SQAUtil::SQAClientPublisher, "reg"));
    ASSERT_STR_EQ(eventData, "regular-event-data");

    // TEST: Empty eventID should not be accepted for publish
    ASSERT_COND(!(publisher->publish("", "regular-event-data", false)));
    // TEST: Empty data should not be accepted for publish
    ASSERT_COND(!(publisher->publish("reg", "", false)));


    // TEST: Publisher can publish other events too
    ASSERT_COND(publisher->publish("other", "other-event-data", true));
    timedTerminateWatcher(100, &watcher);
    //this watcher should receive the termination request and nothing else
    ASSERT_COND(watcher.watch(eventId, eventData));
    ASSERT_STR_EQ(eventId, SQA_TERMINATE_STRING);
    ASSERT_STR_EQ(eventData, SQA_TERMINATE_STRING);

    delete publisher;
}


// Test regular behavior of an external publish()
DEFINE_TEST(TestDriver, TestPublishToExternalBehavior)
{
    StateQueueAgent* _pAgent = GET_RESOURCE(TestDriver, StateQueueAgent*, "state_agent");

    std::string address;
    std::string port;
    _pAgent->options().getOption("sqa-control-address", address);
    _pAgent->options().getOption("sqa-control-port", port);

    // Prepare a publisher and a watcher
    StateQueueClient* publisher = new StateQueueClient(SQAUtil::SQAClientPublisher, "TestPublishToExternalBehavior", address, port, "reg", 1);
    StateQueueClient watcher(SQAUtil::SQAClientWatcher, "StateQueueDriverTest", address, port, "reg", 1);
    // Construct a second watcher for all event to watch for unexpected events
    StateQueueClient watcherAll(SQAUtil::SQAClientWatcher, "StateQueueDriverTest", address, port, "", 1);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    std::string eventId;
    std::string eventData;

    // TEST: Regular no-response publish / watch should work
    ASSERT_COND(publisher->publish("sqw.reg.1111-2222", "external-event-data", true));
    ASSERT_COND(watcher.watch(eventId, eventData));
    // TEST: External publish uses directly the eventId as messageId with no modifications
    ASSERT_STR_EQ(eventId, "sqw.reg.1111-2222");
    //TODO: Verify that the eventId has the proper format with sqw.eventId.hex4-hex4
    ASSERT_STR_EQ(eventData, "external-event-data");
    // second watcher should get this too
    ASSERT_COND(watcherAll.watch(eventId, eventData));

    // TEST: Regular with response publish / watch should work (response is ignored for external)
    ASSERT_COND(publisher->publish("sqw.reg.1111-2222", "external-event-data", false));
    ASSERT_COND(watcher.watch(eventId, eventData));
    ASSERT_STR_EQ(eventId, "sqw.reg.1111-2222");
    //TODO: Verify that the eventId has the proper format with sqw.eventId.hex4-hex4
    ASSERT_STR_EQ(eventData, "external-event-data");
    // second watcher should get this too
    ASSERT_COND(watcherAll.watch(eventId, eventData));

    // TEST: StateQueueAgent will refuse to publish external eventId without proper format
    ASSERT_COND(!publisher->publish("reg", "external-malformed-event-id-data", false));

    // TEST: Empty eventID does not work for external publisher
    ASSERT_COND(!(publisher->publish("", "regular-event-data", false)));

    // TEST: None of the events above were published by StateQueueAgent
    timedTerminateWatcher(200, &watcherAll);
    //this watcher should receive the termination request and nothing else
    ASSERT_COND(watcherAll.watch(eventId, eventData));
    ASSERT_STR_EQ(eventId, SQA_TERMINATE_STRING);
    ASSERT_STR_EQ(eventData, SQA_TERMINATE_STRING);

    delete publisher;
}

//TODO: Add terminate() functionality to publisher, add tests for it
//TODO: Add test  for publish() with empty data

//***********************Publisher Tests*****************************************

//***********************Publisher/Watcher HA Test*****************************************
#if 0
DEFINE_TEST(TestDriver, TestPublisherWatcherHA)
{
  g_driver->generateSQAAgentData(2);
  SQAAgentData::Ptr agentData1 = g_driver->_agents[0];
  //SQAAgentData::Ptr agentData2 = g_driver->_agents[1];

  g_driver->startSQAAgent(agentData1);
  //g_driver->startSQAAgent(agentData2);

  // prepare publisher to local and remote for events of type "reg"
  StateQueueClient publisher1(SQAUtil::SQAClientPublisher, "PublisherLocal", agentData1->sqaControlAddress, agentData1->sqaControlPort, "reg", 1);
  // prepare watchers to local and remote for events of type "reg"
  //StateQueueClient watcher2(SQAUtil::SQAClientWatcher, "WatcherRemote", agentData2->sqaControlAddress, agentData2->sqaControlPort, "reg", 1);


  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  std::string eventId;
  std::string eventData;

  // TEST: Regular no-response publish / watch should work
  ASSERT_COND(publisher1.publish("reg.1", "reg-data-1", true));
  //ASSERT_COND(watcher2.watch(eventId, eventData));
  // TEST: External publish uses directly the eventId as messageId with no modifications
  //ASSERT_STR_STARTS_WITH(eventId, "sqw.reg.1");
  //TODO: Verify that the eventId has the proper format with sqw.eventId.hex4-hex4
  //ASSERT_STR_EQ(eventData, "reg-data-1");
}
#endif
//***********************Publisher/Watcher HA Test*****************************************

//***********************Dealer/Worker HA Test*********************************************
#if 0
DEFINE_TEST(TestDriver, TestDealerWorkerHA)
{
  g_driver->generateSQAAgentData(2);
  SQAAgentData::Ptr agentData1 = g_driver->_agents[0];
  SQAAgentData::Ptr agentData2 = g_driver->_agents[1];

  g_driver->startSQAAgent(agentData1);
  g_driver->startSQAAgent(agentData2);

  // prepare dealer to local and remote for events of type "reg"
  StateQueueClient dealer1(SQAUtil::SQAClientDealer, "DealerLocal", agentData1->sqaControlAddress, agentData1->sqaControlPort, "reg", 1);
  // prepare watchers to local and remote for events of type "reg"
  StateQueueClient worker2(SQAUtil::SQAClientWatcher, "DealerRemote", agentData2->sqaControlAddress, agentData2->sqaControlPort, "reg",  1);


  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  std::string workId;
  std::string workData;

  // TEST: Regular no-response publish / watch should work
  ASSERT_COND(dealer1.enqueue("reg-data-1", 10));
  ASSERT_COND(worker2.pop(workId, workData));
  // TEST: External publish uses directly the eventId as messageId with no modifications
  ASSERT_STR_EQ(workId, "sqa.reg");
  //TODO: Verify that the eventId has the proper format with sqw.eventId.hex4-hex4
  ASSERT_STR_EQ(workData, "reg-data-1");
}
#endif
//***********************Dealer/Worker HA Test*********************************************

//***********************SQAUtil Test*********************************************
DEFINE_TEST(TestDriver, TestSQAUtil)
{
  ASSERT_COND(!SQAUtil::isPublisher(SQAUtil::SQAClientUnknown));
  ASSERT_COND(SQAUtil::isPublisher(SQAUtil::SQAClientPublisher));
  ASSERT_COND(SQAUtil::isPublisher(SQAUtil::SQAClientDealer));
  ASSERT_COND(!SQAUtil::isPublisher(SQAUtil::SQAClientWatcher));
  ASSERT_COND(!SQAUtil::isPublisher(SQAUtil::SQAClientWorker));

  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::SQAClientUnknown));
  ASSERT_COND(SQAUtil::isPublisherOnly(SQAUtil::SQAClientPublisher));
  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::SQAClientDealer));
  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::SQAClientWatcher));
  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::SQAClientWorker));

  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::SQAClientUnknown));
  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::SQAClientPublisher));
  ASSERT_COND(SQAUtil::isDealer(SQAUtil::SQAClientDealer));
  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::SQAClientWatcher));
  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::SQAClientWorker));

  ASSERT_COND(!SQAUtil::isWatcher(SQAUtil::SQAClientUnknown));
  ASSERT_COND(!SQAUtil::isWatcher(SQAUtil::SQAClientPublisher));
  ASSERT_COND(!SQAUtil::isWatcher(SQAUtil::SQAClientDealer));
  ASSERT_COND(SQAUtil::isWatcher(SQAUtil::SQAClientWatcher));
  ASSERT_COND(SQAUtil::isWatcher(SQAUtil::SQAClientWorker));

  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::SQAClientUnknown));
  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::SQAClientPublisher));
  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::SQAClientDealer));
  ASSERT_COND(SQAUtil::isWatcherOnly(SQAUtil::SQAClientWatcher));
  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::SQAClientWorker));

  ASSERT_COND(!SQAUtil::isWorker(SQAUtil::SQAClientUnknown));
  ASSERT_COND(!SQAUtil::isWorker(SQAUtil::SQAClientPublisher));
  ASSERT_COND(!SQAUtil::isWorker(SQAUtil::SQAClientDealer));
  ASSERT_COND(!SQAUtil::isWorker(SQAUtil::SQAClientWatcher));
  ASSERT_COND(SQAUtil::isWorker(SQAUtil::SQAClientWorker));

  ASSERT_STR_EQ("unknown", SQAUtil::getClientStr(SQAUtil::SQAClientUnknown));
  ASSERT_STR_EQ("publisher", SQAUtil::getClientStr(SQAUtil::SQAClientPublisher));
  ASSERT_STR_EQ("dealer", SQAUtil::getClientStr(SQAUtil::SQAClientDealer));
  ASSERT_STR_EQ("watcher", SQAUtil::getClientStr(SQAUtil::SQAClientWatcher));
  ASSERT_STR_EQ("worker", SQAUtil::getClientStr(SQAUtil::SQAClientWorker));

  std::string zmqEventId;
  std::string eventId = "reg";
  ASSERT_COND(SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::SQAClientWatcher, eventId));
  ASSERT_STR_EQ("sqw.reg", zmqEventId);
  ASSERT_COND(SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::SQAClientWorker, eventId));
  ASSERT_STR_EQ("sqa.reg", zmqEventId);
  ASSERT_COND(!SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::SQAClientPublisher, eventId));
  ASSERT_COND(!SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::SQAClientDealer, eventId));


  std::string id;
  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::SQAClientPublisher, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqw.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::SQAClientPublisher, "reg"));

  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::SQAClientDealer, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqa.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::SQAClientDealer, "reg"));

  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::SQAClientWatcher, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqw.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::SQAClientWatcher, "reg"));

  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::SQAClientWorker, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqa.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::SQAClientWorker, "reg"));

  ASSERT_COND(!SQAUtil::generateId(id, SQAUtil::SQAClientUnknown, "reg"));


  //static bool validateId(const std::string &id, int serviceType);
  //static bool validateId(const std::string &id, int serviceType, const std::string &eventId);

  ASSERT_COND(SQAUtil::validateIdHexComponent("1234"));
  ASSERT_COND(SQAUtil::validateIdHexComponent("12D3"));
  ASSERT_COND(SQAUtil::validateIdHexComponent("AABB"));
  ASSERT_COND(!SQAUtil::validateIdHexComponent("a2D3"));
  ASSERT_COND(!SQAUtil::validateIdHexComponent("123"));
  ASSERT_COND(!SQAUtil::validateIdHexComponent("12D322"));

};
//***********************SQAUtil Test*********************************************


bool StateQueueDriverTest::runTests()
{
  g_driver = this;
  std::string address;
  std::string port;

  _agent.options().getOption("sqa-control-address", address);
  _agent.options().getOption("sqa-control-port", port);

  //
  // Define common resource accessible by all unit tests
  //
  DEFINE_RESOURCE(TestDriver, "state_agent", &_agent);
  DEFINE_RESOURCE(TestDriver, "argc", _argc);
  DEFINE_RESOURCE(TestDriver, "argv", _argv);
  DEFINE_RESOURCE(TestDriver, "simple_pop_client", new StateQueueClient(SQAUtil::SQAClientWorker, "StateQueueDriverTest", address, port, "reg", 2));
  DEFINE_RESOURCE(TestDriver, "simple_publisher", new StateQueueClient(SQAUtil::SQAClientPublisher, "StateQueueDriverTest", address, port, "reg", 2));


  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
//
//  boost::thread* _pIoServiceThread;
//  boost::asio::io_service _ioService;
//  _pIoServiceThread = new boost::thread(boost::bind(&boost::asio::io_service::run, &_ioService));
//  StateQueueClient::SQAClientCore::BlockingTcpClient* pClient= new StateQueueClient::SQAClientCore::BlockingTcpClient(_ioService, 100, 100, SQA_KEY_ALPHA);
//  pClient->connect(address, port);
//
//  StateQueueMessage ping;
//  StateQueueMessage pong;
//  ping.setType(StateQueueMessage::Ping);
//  std::string _applicationId = "lol";
//  ping.set("message-app-id", _applicationId.c_str());
//
//  if (!pClient->isConnected() && !pClient->connect(address, port))
//  {
//    OS_LOG_DEBUG(FAC_NET, "Client is not connected to  " << address << ":" << port);
//    return false;
//  }
//
//  bool sent = pClient->sendAndReceive(ping, pong);
//  if (sent)
//  {
//    if (pong.getType() == StateQueueMessage::Pong)
//    {
//      OS_LOG_DEBUG(FAC_NET, "Keep-alive response received from " << address << ":" << port);
//    }
//  }
//  return true;


  //
  // Run the unit tests
  //
//    VERIFY_TEST(TestDriver, TestTimedMap);
//    VERIFY_TEST(TestDriver, TestMapGetSet);
//    VERIFY_TEST(TestDriver, TestMapGetSetPlugin)
//    VERIFY_TEST(TestDriver, TestSimplePop);
//    VERIFY_TEST(TestDriver, TestMultiplePop);
//    VERIFY_TEST(TestDriver, TestGetSetErase);
//    VERIFY_TEST(TestDriver, TestSimplePersistGetErase);
//    VERIFY_TEST(TestDriver, TestWatcher);
//    VERIFY_TEST(TestDriver, TestPublishAndSet);
//    VERIFY_TEST(TestDriver, TestDealAndPublish)

//    VERIFY_TEST(TestDriver, TestPublishNoConnection);
//    VERIFY_TEST(TestDriver, TestPublishRestrictionToNonPublishers);
//    VERIFY_TEST(TestDriver, TestPublishRegularBehavior);
//    VERIFY_TEST(TestDriver, TestPublishToExternalBehavior);

    //VERIFY_TEST(TestDriver, TestSQAUtil);
    //VERIFY_TEST(TestDriver, TestPublisherWatcherHA);
    //VERIFY_TEST(TestDriver, TestDealerWorkerHA );

  //
  // Delete simple_pop_client so it does not participate in popping events
  //
  delete GET_RESOURCE(TestDriver, StateQueueClient*, "simple_pop_client");
  //VERIFY_TEST(TestDriver, TestMultiplePop);
  //
  // Delete the common resource because the are heap allocated using new()!
  //
  delete GET_RESOURCE(TestDriver, StateQueueClient*, "simple_publisher");
  

  END_UNIT_TEST(TestDriver);

  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  return TEST_RESULT(TestDriver);
}
