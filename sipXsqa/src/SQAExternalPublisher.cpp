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

#include "sqa/SQAExternalPublisher.h"


bool SQAExternalPublisher::start()
{
    OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::start"
        << " [" << this << "]");

    // use thread handle to see if this was already started
    if (_publishingThread)
    {
        OS_LOG_NOTICE(FAC_NET, "SQAExternalPublisher::start"
            << " [" << this << "]"
            << " already started");
        return true;
    }

    bool ret = true;

    std::string mySqaControlAddress;
    std::string mySqaControlPort;

    // extract control addresses and ports for all SQA agents
    if (!_options.getOption("sqa-control-address", mySqaControlAddress) ||
            !_options.getOption("sqa-control-port", mySqaControlPort))
    {
        OS_LOG_ERROR(FAC_NET, "SQAExternalPublisher::start this: " << this <<
                " could not find sqa-control-address or sqa-control-port");
        ret = false;
    }

    if (true == ret)
    {
        if (mySqaControlAddress.empty() || mySqaControlPort.empty())
        {
            OS_LOG_ERROR(FAC_NET, "SQAExternalPublisher::start this: " << this <<
                    " sqa-control-address or sqa-control-port is empty");
            ret = false;
        }
    }

    std::string sqaControlAddressAll;
    std::string sqaControlPortAll;
    if (true == ret)
    {
        // extract control addresses and ports for all SQA agents
        if (!_options.getOption("sqa-control-address-all", sqaControlAddressAll) ||
                !_options.getOption("sqa-control-port-all", sqaControlPortAll))
        {
            OS_LOG_ERROR(FAC_NET, "SQAExternalPublisher::start this: " << this <<
                    " could not get sqa-control-address-all or sqa-control-port-all");
            ret = false;
        }
    }

    if (true == ret)
    {
        if (sqaControlAddressAll.empty() || sqaControlPortAll.empty())
        {
            OS_LOG_ERROR(FAC_NET, "SQAExternalPublisher::start"
                    " sqa-control-address-all or sqa-control-port-all is empty");
            ret = false;
        }
    }

    OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::start"
        << " [" << this << "]"
        << " myControlAddress: " << mySqaControlAddress << ":" << mySqaControlPort << ";"
        << " allControlAddresses: " << sqaControlAddressAll << ":" << sqaControlPortAll);

    std::vector<std::string> addresses;
    std::vector<std::string> ports;
    if (true == ret)
    {
        boost::algorithm::split(addresses, sqaControlAddressAll, boost::is_any_of(","), boost::token_compress_on);
        boost::algorithm::split(ports, sqaControlPortAll, boost::is_any_of(","), boost::token_compress_on);

        if (addresses.size() != ports.size())
        {
            OS_LOG_ERROR(FAC_NET, "SQAExternalPublisher::start this: " << this <<
                    " cannot match addresses with ports");
            ret = false;
        }
    }

    if (true == ret)
    {
        //for each available SQA agent create a publisher
        int externalsCount = addresses.size();
        for(int i = 0; i < externalsCount; i++)
        {
            if (addresses[i] == mySqaControlAddress && ports[i] == mySqaControlPort)
            {
                // no need to create a publisher to our own agent address
                continue;
            }
            if (addresses[i].empty() || ports[i].empty())
            {
              OS_LOG_NOTICE(FAC_NET, "SQAExternalPublisher::start this: " << this <<
                  " Empty address: " << addresses[i] << ":" << ports[i]);
                continue;
            }

            std::ostringstream ss;
            ss << "SQAPublisherExternal-" << mySqaControlAddress << ":" << mySqaControlPort << "-to-" << addresses[i] << ":" << ports[i];

            SQAPublisher* publisher = new SQAPublisher(ss.str().c_str(), addresses[i].c_str(), ports[i].c_str(), true, 1, 100, 100);
            OS_LOG_AND_ASSERT(publisher, FAC_NET, "SQAExternalPublisher::start"
                    " failed to allocate publisher pointer");

            _publishers.push_back(publisher);

            OS_LOG_INFO(FAC_NET, "SQAExternalPublisher::start this: " << this <<
                    " created publisher to external address:" << addresses[i] << ":" << ports[i]
                    << " , is connected: " << publisher->isConnected());

        }

        _publishingThread = new boost::thread(boost::bind(&SQAExternalPublisher::run, this));
        OS_LOG_AND_ASSERT(_publishingThread, FAC_NET, "SQAExternalPublisher::start"
                " failed to allocate _publishingThread pointer");
    }

    return ret;
}

void SQAExternalPublisher::stop()
{
    OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::stop this: " << this <<
            " already started");

    if (!_publishingThread || !_publishingThread->joinable())
    {
        return;
    }

    //mark _publishingThread for termination
    _isRunning = false;

    // signal condition to unblock the queue
    _cond.notify_one();

    // wait for thread to finish
    _publishingThread->join();

    // destroy all publishers to external agents
    std::vector<SQAPublisher*>::iterator it;
    for (it = _publishers.begin(); it != _publishers.end(); it++)
    {
        SQAPublisher *publisher = *it;
        delete publisher;
    }

    _publishers.clear();

    // clear the queue
    while (!_queue.empty())
    {
        _queue.pop();
    }
}
