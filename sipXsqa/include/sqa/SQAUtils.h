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

extern const char* serviceTypeStr[];
extern const char* connectionEventStr[];

const char* getServiceTypeStr(ServiceType serviceType);
const char* getConnectionEventStr(ConnectionEvent connectionEvent);

void generateRecordId(std::string &recordId, ConnectionEvent event);

bool generateZmqEventId(std::string &zmqEventId, ServiceType serviceType, std::string &eventId);

bool generateId(std::string &id, ServiceType serviceType, const std::string &eventId);

bool validateId(const std::string &id, ServiceType serviceType);
bool validateId(const std::string &id, ServiceType serviceType, const std::string &eventId);
bool validateIdHexComponent(const std::string &hex);

#endif //SQAUTILS_H_INCLUDED
