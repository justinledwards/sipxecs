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

#ifndef SQAUTILS_H_INCLUDED
#define SQAUTILS_H_INCLUDED

#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include "sqa/SQADefines.h"

extern const char* connectionEventStr[];

#define LOG_TAG_WID(id) this->getClassName() << "::" << __FUNCTION__ << "." << __LINE__ << " this:" << this << " id:" << id
#define LOG_TAG() this->getClassName() << "::" << __FUNCTION__ << "." << __LINE__ << " this:" << this

class SQAUtil
{
public:
  static const int ServiceUnknown;
  static const int ServicePublisher;
  static const int ServiceDealer;
  static const int ServiceWatcher;
  static const int ServiceWorker;
  static const int ServiceWorkerMulti;

  static const int ServiceRolePublisher = 0x1;
  static const int ServiceRoleWatcher = 0x10;

  static const int ServiceSpecDealer = 0x1000;
  static const int ServiceSpecWorker = 0x10000;
  static const int ServiceSpecMulti  = 0x100000;
  static const int ServiceSpecExternal= 0x1000000;

  static bool isExternal(int serviceType)
  {
    return (serviceType & ServiceSpecExternal);
  }

  static bool isPublisher(int serviceType)
  {
    return (serviceType & ServiceRolePublisher);
  }

  static bool isPublisherOnly(int serviceType)
  {
    return ((serviceType & ServiceRolePublisher) && !(serviceType & ServiceSpecDealer));
  }

  static bool isDealer(int serviceType)
  {
    return (serviceType & ServiceRolePublisher && serviceType & ServiceSpecDealer);
  }

  static bool isWatcher(int serviceType)
  {
    return (serviceType & ServiceRoleWatcher);
  }

  static bool isWatcherOnly(int serviceType)
  {
    return (serviceType == ServiceRoleWatcher);
  }

  static bool isWorker(int serviceType)
  {
    return (serviceType & ServiceRoleWatcher && serviceType & ServiceSpecWorker);
  }

  static bool isMulti(int serviceType)
  {
    return (serviceType & ServiceSpecMulti);
  }

  static const char* getServiceTypeStr(int serviceType);
  static const char* getConnectionEventStr(ConnectionEvent connectionEvent);

  static void generateRecordId(std::string &recordId, ConnectionEvent event);

  static bool generateZmqEventId(std::string &zmqEventId, int serviceType, std::string &eventId);

  static bool generateId(std::string &id, int serviceType, const std::string &eventId);

  static bool validateId(const std::string &id, int serviceType);
  static bool validateId(const std::string &id, int serviceType, const std::string &eventId);
  static bool validateIdHexComponent(const std::string &hex);

};


#endif //SQAUTILS_H_INCLUDED
