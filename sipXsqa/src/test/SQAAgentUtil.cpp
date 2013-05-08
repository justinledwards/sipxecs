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

#include "SQAAgentUtil.h"
#include <iostream>
#include "sqa/UnitTest.h"

const char* g_defaultSqaControlAddress = "192.168.1.57";
const char* g_defaultZmqSubscriptionAddress = "192.168.1.57";

const unsigned int g_defaultSqaControlPort = 6240;
const unsigned int g_defaultZmqSubscriptionPort = 6242;
const unsigned int g_portIncrement = 100;

SQAAgentUtil *g_driver = 0;

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

#if 0
//***********************Publisher/Watcher HA Test*****************************************
DEFINE_TEST(TestDriver, TestPublisherWatcherHA)
{
  g_driver->generateSQAAgentData(2);
  SQAAgentData::Ptr agentData1 = g_driver->_agents[0];
  SQAAgentData::Ptr agentData2 = g_driver->_agents[1];

  g_driver->startSQAAgent(agentData1);
  g_driver->startSQAAgent(agentData2);

  // prepare publisher to local and remote for events of type "reg"
  StateQueueClient publisher1(SQAUtil::ServicePublisher, "PublisherLocal", agentData1->sqaControlAddress, agentData1->sqaControlPort, "reg", 1);
  // prepare watchers to local and remote for events of type "reg"
  StateQueueClient watcher2(SQAUtil::ServiceWatcher, "WatcherRemote", agentData2->sqaControlAddress, agentData2->sqaControlPort, "reg", 1);


  boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

  std::string eventId;
  std::string eventData;

  // TEST: Regular no-response publish / watch should work
  ASSERT_COND(publisher1.publish("reg.1", "reg-data-1", true));
  ASSERT_COND(watcher2.watch(eventId, eventData));
  // TEST: External publish uses directly the eventId as messageId with no modifications
  ASSERT_STR_STARTS_WITH(eventId, "sqw.reg.1");
  //TODO: Verify that the eventId has the proper format with sqw.eventId.hex4-hex4
  ASSERT_STR_EQ(eventData, "reg-data-1");
}
//***********************Publisher/Watcher HA Test*****************************************

//***********************Dealer/Worker HA Test*********************************************
DEFINE_TEST(TestDriver, TestDealerWorkerHA)
{
  g_driver->generateSQAAgentData(2);
  SQAAgentData::Ptr agentData1 = g_driver->_agents[0];
  SQAAgentData::Ptr agentData2 = g_driver->_agents[1];

  g_driver->startSQAAgent(agentData1);
  g_driver->startSQAAgent(agentData2);

  // prepare dealer to local and remote for events of type "reg"
  StateQueueClient dealer1(SQAUtil::ServiceDealer, "DealerLocal", agentData1->sqaControlAddress, agentData1->sqaControlPort, "reg", 1);
  // prepare watchers to local and remote for events of type "reg"
  StateQueueClient worker2(SQAUtil::ServiceWatcher, "DealerRemote", agentData2->sqaControlAddress, agentData2->sqaControlPort, "reg",  1);


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

#if 0
//***********************SQAUtil Test*********************************************
DEFINE_TEST(TestDriver, TestSQAUtil)
{
  ASSERT_COND(!SQAUtil::isPublisher(SQAUtil::ServiceUnknown));
  ASSERT_COND(SQAUtil::isPublisher(SQAUtil::ServicePublisher));
  ASSERT_COND(SQAUtil::isPublisher(SQAUtil::ServiceDealer));
  ASSERT_COND(!SQAUtil::isPublisher(SQAUtil::ServiceWatcher));
  ASSERT_COND(!SQAUtil::isPublisher(SQAUtil::ServiceWorker));
  ASSERT_COND(!SQAUtil::isPublisher(SQAUtil::ServiceWorkerMulti));

  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::ServiceUnknown));
  ASSERT_COND(SQAUtil::isPublisherOnly(SQAUtil::ServicePublisher));
  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::ServiceDealer));
  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::ServiceWatcher));
  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::ServiceWorker));
  ASSERT_COND(!SQAUtil::isPublisherOnly(SQAUtil::ServiceWorkerMulti));

  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::ServiceUnknown));
  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::ServicePublisher));
  ASSERT_COND(SQAUtil::isDealer(SQAUtil::ServiceDealer));
  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::ServiceWatcher));
  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::ServiceWorker));
  ASSERT_COND(!SQAUtil::isDealer(SQAUtil::ServiceWorkerMulti));

  ASSERT_COND(!SQAUtil::isWatcher(SQAUtil::ServiceUnknown));
  ASSERT_COND(!SQAUtil::isWatcher(SQAUtil::ServicePublisher));
  ASSERT_COND(!SQAUtil::isWatcher(SQAUtil::ServiceDealer));
  ASSERT_COND(SQAUtil::isWatcher(SQAUtil::ServiceWatcher));
  ASSERT_COND(SQAUtil::isWatcher(SQAUtil::ServiceWorker));
  ASSERT_COND(SQAUtil::isWatcher(SQAUtil::ServiceWorkerMulti));

  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::ServiceUnknown));
  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::ServicePublisher));
  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::ServiceDealer));
  ASSERT_COND(SQAUtil::isWatcherOnly(SQAUtil::ServiceWatcher));
  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::ServiceWorker));
  ASSERT_COND(!SQAUtil::isWatcherOnly(SQAUtil::ServiceWorkerMulti));

  ASSERT_COND(!SQAUtil::isWorker(SQAUtil::ServiceUnknown));
  ASSERT_COND(!SQAUtil::isWorker(SQAUtil::ServicePublisher));
  ASSERT_COND(!SQAUtil::isWorker(SQAUtil::ServiceDealer));
  ASSERT_COND(!SQAUtil::isWorker(SQAUtil::ServiceWatcher));
  ASSERT_COND(SQAUtil::isWorker(SQAUtil::ServiceWorker));
  ASSERT_COND(SQAUtil::isWorker(SQAUtil::ServiceWorkerMulti));

  ASSERT_COND(!SQAUtil::isExternal(SQAUtil::ServicePublisher));
  ASSERT_COND(SQAUtil::isExternal(SQAUtil::ServicePublisher | SQAUtil::ServiceSpecExternal));
  ASSERT_COND(!SQAUtil::isExternal(SQAUtil::ServiceDealer));
  ASSERT_COND(SQAUtil::isExternal(SQAUtil::ServicePublisher | SQAUtil::ServiceSpecExternal));

  ASSERT_STR_EQ("unknown", SQAUtil::getServiceTypeStr(SQAUtil::ServiceUnknown));
  ASSERT_STR_EQ("publisher", SQAUtil::getServiceTypeStr(SQAUtil::ServicePublisher));
  ASSERT_STR_EQ("dealer", SQAUtil::getServiceTypeStr(SQAUtil::ServiceDealer));
  ASSERT_STR_EQ("watcher", SQAUtil::getServiceTypeStr(SQAUtil::ServiceWatcher));
  ASSERT_STR_EQ("worker", SQAUtil::getServiceTypeStr(SQAUtil::ServiceWorker));
  ASSERT_STR_EQ("worker", SQAUtil::getServiceTypeStr(SQAUtil::ServiceWorkerMulti));

  std::string zmqEventId;
  std::string eventId = "reg";
  ASSERT_COND(SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::ServiceWatcher, eventId));
  ASSERT_STR_EQ("sqw.reg", zmqEventId);
  ASSERT_COND(SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::ServiceWorker, eventId));
  ASSERT_STR_EQ("sqa.reg", zmqEventId);
  ASSERT_COND(SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::ServiceWorkerMulti, eventId));
  ASSERT_STR_EQ("sqa.reg", zmqEventId);
  ASSERT_COND(!SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::ServicePublisher, eventId));
  ASSERT_COND(!SQAUtil::generateZmqEventId(zmqEventId, SQAUtil::ServiceDealer, eventId));


  std::string id;
  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::ServicePublisher, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqw.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::ServicePublisher, "reg"));

  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::ServiceDealer, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqa.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::ServiceDealer, "reg"));

  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::ServiceWatcher, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqw.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::ServiceWatcher, "reg"));

  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::ServiceWorker, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqa.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::ServiceWorker, "reg"));

  ASSERT_COND(SQAUtil::generateId(id, SQAUtil::ServiceWorkerMulti, "reg"));
  ASSERT_STR_STARTS_WITH(id, "sqa.reg");
  ASSERT_COND(SQAUtil::validateId(id, SQAUtil::ServiceWorkerMulti, "reg"));

  ASSERT_COND(!SQAUtil::generateId(id, SQAUtil::ServiceUnknown, "reg"));


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
#endif

SQAAgentUtil::SQAAgentUtil() :
     _configFileIdx(0)
{
}


SQAAgentUtil::SQAAgentUtil(const std::string& program) :
     _program(program), _configFileIdx(0)
{
}

SQAAgentUtil::~SQAAgentUtil()
{
}

void SQAAgentUtil::setProgram(const std::string& program)
{
     _program == program;
}


void SQAAgentUtil::generateSQAAgentData(std::vector<SQAAgentData::Ptr>& agents, unsigned int agentsNum, bool ha)
{
  agents.clear();

  std::string sqaControlAddressAll;
  std::string sqaControlPortAll;

  for (unsigned int i = 0; i < agentsNum; i++)
  {
    SQAAgentData::Ptr data = SQAAgentData::Ptr(new SQAAgentData());

    {
      std::stringstream strm;
      strm << "SQATestAgent" << i;
      data->id = strm.str();
    }

    {
      std::stringstream strm;
      strm << (g_defaultSqaControlPort + g_portIncrement * i);
      data->sqaControlPort = strm.str();
    }
    {
      std::stringstream strm;
      strm << (g_defaultZmqSubscriptionPort + g_portIncrement * i);
      data->sqaZmqSubscriptionPort = strm.str();
    }

    data->sqaControlAddress = g_defaultSqaControlAddress;
    data->sqaZmqSubscriptionAddress = g_defaultZmqSubscriptionAddress;

    {
      std::stringstream strm;
      strm << "sipxsqa-config-" << i;
      data->configFilePath = strm.str();
    }

    {
      std::stringstream strm;
      strm << "sipxsqa.log-" << i;
      data->logFilePath = strm.str();
    }

    if (ha)
    {
      sqaControlAddressAll += data->sqaControlAddress + ",";
      sqaControlPortAll += data->sqaControlPort + ",";
    }

    data->ha = ha;

    agents.push_back(data);
  }

  std::vector<SQAAgentData::Ptr>::iterator it;
  for (it = agents.begin(); it != agents.end(); it++)
  {
    SQAAgentData::Ptr data = *it;

    if (ha)
    {
      data->sqaControlPortAll = sqaControlPortAll;
      data->sqaControlAddressAll = sqaControlAddressAll;
    }

    generateSQAConfig(data);
  }
}

void SQAAgentUtil::generateSQAConfig(SQAAgentData::Ptr data)
{
  std::ofstream ofs(data->configFilePath.data(), std::ios_base::trunc | std::ios_base::in);

  ofs << "log-level=7" << "\n"
      << "sqa-control-port=" << data->sqaControlPort << "\n"
      << "zmq-subscription-port=" << data->sqaZmqSubscriptionPort << "\n"
      << "sqa-control-address=" << data->sqaControlAddress << "\n"
      << "zmq-subscription-address=" << data->sqaZmqSubscriptionAddress <<"\n";

  if (!data->sqaControlPortAll.empty())
  {
    ofs << "sqa-control-port-all=" << data->sqaControlPortAll << "\n";
  }
  if (!data->sqaControlAddressAll.empty())
  {
    ofs << "sqa-control-address-all=" << data->sqaControlAddressAll << "\n";
  }
}

bool SQAAgentUtil::startSQAAgent(SQAAgentData::Ptr agentData)
{
  int argc = 5;
  const char *argv[argc];
  argv[0] = _program.c_str();
  argv[1] = "--config-file";
  argv[2] = agentData->configFilePath.data();
  argv[3] = "--log-file";
  argv[4] = agentData->logFilePath.data();

  agentData->service= new ServiceOptions(argc, (char**)argv, agentData->id);
  agentData->service->addDaemonOptions();
  agentData->service->addOptionString("zmq-subscription-address", ": Address where to subscribe for events.");
  agentData->service->addOptionString("zmq-subscription-port", ": Port where to send subscription for events.");
  agentData->service->addOptionString("sqa-control-port", ": Port where to send control commands.");
  agentData->service->addOptionString("sqa-control-address", ": Address where to send control commands.");

  if (!agentData->service->parseOptions() ||
          !agentData->service->hasOption("zmq-subscription-address") ||
          !agentData->service->hasOption("zmq-subscription-port") ||
          !agentData->service->hasOption("sqa-control-port") ||
          !agentData->service->hasOption("sqa-control-address") )
  {
    agentData->service->displayUsage(std::cerr);
    return false;
  }

  if (agentData->ha)
  {
    pid_t pid = fork();
    if (pid == 0)                // child
    {
    // Code only executed by child process
      agentData->agent = new StateQueueAgent(agentData->id, *agentData->service);
      agentData->agent->run();

      agentData->service->waitForTerminationRequest();
      delete agentData->agent;
      delete agentData->service;
      exit(1);
    }
    else if (pid < 0)            // failed to fork
    {
      return false;
    }
    else                                   // parent
    {
      agentData->pid = pid;
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }
  }
  else
  {
    agentData->agent = new StateQueueAgent(agentData->id, *agentData->service);
    agentData->agent->run();
  }

  return true;;
}
#include <sys/types.h>
#include <sys/wait.h>
bool SQAAgentUtil::stopSQAAgent(SQAAgentData::Ptr data)
{
  if (data->ha)
  {
    if (data->pid > 0)
    {
      kill(data->pid, SIGTERM);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1));
      kill(data->pid, SIGTERM);
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));

      int status;
      pid_t pid = waitpid(data->pid, &status, WNOHANG);
      if (pid == data->pid)
      {
        return true;
      }
    }

    return false;
  }
  else
  {
    if (data->agent)
    {
      delete data->agent;
      data->agent = 0;
    }
    else
    {
      return false;
    }

    if (data->service)
    {
      delete data->service;
      data->service = 0;
    }
    else
    {
      return false;
    }

    return true;
  }
}
