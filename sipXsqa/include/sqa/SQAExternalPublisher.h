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

// SQAExternalPublisher.h
//
// This header contains the SQAExternalPublisher class.

#ifndef SQA_EXTERNAL_PUBLISHER_H_INCLUDED
#define SQA_EXTERNAL_PUBLISHER_H_INCLUDED

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <cassert>
#include <boost/thread.hpp>
#include "SQAUtils.h"

#include "ServiceOptions.h"
#include "sqaclient.h"
#include "StateQueueRecord.h"


#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>


class SQAExternalPublisher : boost::noncopyable
    /// This class implements a publisher which can publish to a number of external
    /// SQA agents (external means they are located on different hosts).
    ///
    /// This class is intended to be used by StateQueueAgent instance in order to
    /// expand its publishing capability to any number of SQA agents.
    /// A SQAExternalPublisher instance will receive in its constructor the list of
    /// addresses and ports of external SQA to connect to. When method start() is
    /// called it will create internal publisher objects to the SQA agents.
    ///
    /// The publish() method will publish the given record to all connected SQA agents.
{
public:
    SQAExternalPublisher(
            ServiceOptions& options,
            unsigned int maxQueueSize = 0);
    ~SQAExternalPublisher();

    bool start();
    /// Start the external publisher.
    /// This method will validate the given SQA agent addresses and ports and
    /// for each agent it will create an internal publisher targeted at that
    /// agent's address:port. When it finished constructing publishers it will
    /// run a publishing thread which will take care of publishing events.

    void stop();
    /// Stop the external publisher.
    /// This method is the reverse of start and it will stop and destroy the publishing
    /// thread, destroy all internal publishers, and clear the queue.

    void publish(const StateQueueRecord& record);
    /// Publish the given record to all external SQA agents.
    /// Publishing to external agents is 1:n relation between one local SQA and several
    /// external SQA. To not impact performance of the local SQA a queue is used to
    /// temporarily store given records before publishing. That is why this method is
    /// in fact an enqueue(), the real publishing will take place on the run() method
    /// of the publishing thread.
    /// In case the publishing to externals is slow and the queue gets full (in case a
    /// max queue size was given) it will pop and discard old records to be able to
    /// enqueue new records. Discarded records will not be published.

private:
    void external_publish(StateQueueRecord& record);
    /// This is a helper for publish() method.
    /// It will iterate through all external publishers and it will call their own
    /// publish() on the given record.

    bool dequeue(StateQueueRecord& record);
    /// Retrieve a record from the queue for publishing.
    /// It will return false in case there is nothing to retrieve.

    void run();
    /// This is the publishing thread run function.

    ServiceOptions _options;    /// Config options: addresses and ports for local and external SQAs
    unsigned int _maxQueueSize; /// The maximum allowed queue size or 0 for unlimited queue.

    std::vector<SQAPublisher*> _publishers; /// Vector with the external publishers

    boost::thread* _publishingThread;   /// Thread used for the actual publishing
    bool _isRunning;                    /// True if the thread is running, false after stop()

    boost::condition_variable _cond;    /// Condition var used to sync access to _queue
    boost::mutex _mutex;                /// Mutex var used to sync access to _queue
    std::queue<StateQueueRecord> _queue;/// Internal queue used to temporarily store records
                                        /// before publishing
};


inline SQAExternalPublisher::SQAExternalPublisher(ServiceOptions& options, unsigned int maxQueueSize) :
        _options(options),
        _maxQueueSize(maxQueueSize),
        _publishingThread(0),
        _isRunning(false)
{
    OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::SQAExternalPublisher this: " << this);
}

inline SQAExternalPublisher::~SQAExternalPublisher()
{
    OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::~SQAExternalPublisher this: " << this);

    stop();

    if (_publishingThread)
    {
        delete _publishingThread;
        _publishingThread = 0;
    }
}

inline void SQAExternalPublisher::run()
{
    OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::run this: " << this);

    OS_LOG_AND_ASSERT(!_isRunning, FAC_NET, "SQAExternalPublisher::run"
            " _publishingThread func already running");
    OS_LOG_AND_ASSERT(_publishingThread, FAC_NET, "SQAExternalPublisher::run"
            " _publishingThread pointer not valid (thread not started?)");

    // just loop forever, waiting for records to publish to all SQA agents
    _isRunning = true;
    while(_isRunning)
    {
        StateQueueRecord record;
        if (dequeue(record))
        {
            OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::run"
                << " [" << this << "]"
                << " external publish for record: id:" << record.id << " data:" << record.data);
            external_publish(record);
        }
    }
}

inline void SQAExternalPublisher::external_publish(StateQueueRecord& record)
{
    std::vector<SQAPublisher*>::iterator it;
    for (it = _publishers.begin(); it != _publishers.end(); it++)
    {
        SQAPublisher *publisher = *it;
        bool ret = publisher->publish(record.id.c_str(), record.data.c_str(), true);

        OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::external_publish"
            << " [" << this << "]"
            << " publish result: " << ret );
    }
}

inline bool SQAExternalPublisher::dequeue(StateQueueRecord& record)
{
    boost::unique_lock<boost::mutex> lock(_mutex);

    _cond.wait(lock);
    if (_queue.empty())
    {
        OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::dequeue this: " << this <<
                " empty");
        return false;
    }

    record = _queue.front();
    _queue.pop();

    OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::dequeue this: "
        << " [" << this << "]"
        << " dequeued record: id:" << record.id << " data:" << record.data);
    return true;
}

inline void SQAExternalPublisher::publish(const StateQueueRecord& record)
{
    // do not allow publishing if the thread is not running
    if (!_isRunning)
    {
        return;
    }

    boost::lock_guard<boost::mutex> lock(_mutex);

    if (_maxQueueSize && _queue.size() > _maxQueueSize)
    {
        OS_LOG_WARNING(FAC_NET, "SQAExternalPublisher::publish"
            << " [" << this << "]"
            << " queue is overloaded, maxQueueSize " << _maxQueueSize << " was reached");
        _queue.pop(); // make room for current record
    }

    OS_LOG_DEBUG(FAC_NET, "SQAExternalPublisher::publish this: "
        << " [" << this << "]"
        << " enqueued record: id:" << record.id << " data:" << record.data);

    _queue.push(record);
    _cond.notify_one();
}

#endif //SQA_EXTERNAL_PUBLISHER_H_INCLUDED
