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



#include "sqa/ServiceOptions.h"
#include "sqa/StateQueueAgent.h"
#include "sqa/StateQueueDriverTest.h"
#include "sqa/StateQueueConnection.h"

int main(int argc, char** argv)
{
  std::stringstream strm;
  char a[100] = "1234567890";
  strm.write(a, 10);
  char b;
  std::cout << "size is:" <<  strm.str().size() << std::endl;
  strm.read((char*)&b, sizeof(b));
  std::cout << "size is:" <<  strm.str().size() << std::endl;



  ServiceOptions::daemonize(argc, argv);

  ServiceOptions service(argc, argv, "StateQueueAgent", "1.0.0", "Copyright Ezuce Inc. (All Rights Reserved)");
  service.addDaemonOptions();
  service.addOptionString("zmq-subscription-address", ": Address where to subscribe for events.");
  service.addOptionString("zmq-subscription-port", ": Port where to send subscription for events.");
  service.addOptionString("sqa-control-port", ": Port where to send control commands.");
  service.addOptionString("sqa-control-address", ": Address where to send control commands.");
  service.addOptionInt("id", ": Address where to send control commands.");
  service.addOptionFlag("test-driver", ": Set this flag if you want to run the driver unit tests to ensure proper operations.");
  


  if (!service.parseOptions() ||
          !service.hasOption("zmq-subscription-address") ||
          !service.hasOption("zmq-subscription-port") ||
          !service.hasOption("sqa-control-port") ||
          !service.hasOption("sqa-control-address") )
  {
    service.displayUsage(std::cerr);
    return -1;
  }

  StateQueueAgent sqa(service);
  sqa.run();

#if 0
  int id = 0;
  service.getOption("id", id);

  if (id == 1)
  {
      std::string address="192.168.13.2";
      std::string port="5240";
      SQAPublisher publisher("THEPublisher", address.c_str(), port.c_str(), false, 1, 100, 100);
      while (1)
      {
      boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
      if (true != publisher.publish("pub", "test-data", true))
          printf("lol1\n");
      }
  }
  else if (id == 2)
  {
      std::string address="192.168.13.2";
      std::string port="6240";

      SQAWatcher watcher("THEWatcher", address.c_str(), port.c_str(), "", 1, 100, 100);

      while(1)
      {
    SQAEvent* pEvent = watcher.watch();
    if (!pEvent)
        printf("lol2\n");
    else
        printf("%s %s\n", pEvent->id, pEvent->data);
    delete pEvent;
      }
  }
#endif

  if (service.hasOption("test-driver"))
  {
    StateQueueDriverTest test(sqa, argc, argv);
    if (!test.runTests())
      return -1;
    sqa.stop();
    return 0;
  }
  OS_LOG_INFO(FAC_NET, "State Queue Agent process STARTED.");
  service.waitForTerminationRequest();
  OS_LOG_INFO(FAC_NET, "State Queue Agent process TERMINATED.");
  return 0;
}

