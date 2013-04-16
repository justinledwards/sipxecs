#include "sqa/SQAUtils.h"

const char* serviceTypeStr[] =
{
    "unknown",
    "publisher",
    "dealer",
    "worker",
    "watcher",
};

const char * connectionEventStr[] =
{
    "unknown",
    "established",
    "sigin",
    "keepalive",
    "logout",
    "terminate",
};

const char* getServiceTypeStr(ServiceType serviceType)
{
    if (serviceType >= ServiceTypeNum)
    {
        serviceType = ServiceTypeUnknown;
    }

    return serviceTypeStr[serviceType];
}

const char* getConnectionEventStr(ConnectionEvent connectionEvent)
{
    if (connectionEvent >= ConnectionEventNum)
    {
        connectionEvent = ConnectionEventUnknown;
    }

    return connectionEventStr[connectionEvent];
}


void generateRecordId(std::string &recordId, ConnectionEvent connectionEvent)
{
    recordId = PublisherWatcherPrefix;
    recordId += ".connection.";
    recordId += getConnectionEventStr(connectionEvent);
}

bool generateZmqEventId(std::string &zmqEventId, ServiceType serviceType, std::string &eventId)
{
    bool ret = true;

    switch (serviceType)
    {
    case ServiceTypeWatcher:
        zmqEventId = PublisherWatcherPrefix;
        zmqEventId += "." + eventId;
        break;
    case ServiceTypeWorker:
        zmqEventId = DealerWorkerPrefix;
        zmqEventId += "." + eventId;
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

bool generateId(std::string &id, ServiceType serviceType, const std::string &eventId)
{
    bool ret = true;
    std::ostringstream ss;

    switch(serviceType)
    {
    case ServiceTypePublisher:
    case ServiceTypeWatcher:
        ss << PublisherWatcherPrefix;
        break;
    case ServiceTypeDealer:
    case ServiceTypeWorker:
        ss << DealerWorkerPrefix;
        break;
    case ServiceTypeUnknown:
    default:
        ss << UnknownPrefix;
        ret = false;
        break;
    }

    if (!eventId.empty())
    {
        ss << "." << eventId;
    }

    ss << "." << std::hex << std::uppercase
            << std::setw(4) << std::setfill('0') << (int) ((float) (0x10000) * random () / (RAND_MAX + 1.0)) << "-"
            << std::setw(4) << std::setfill('0') << (int) ((float) (0x10000) * random () / (RAND_MAX + 1.0));

    id = ss.str();

    return ret;
}


bool validateId(const std::string &id, ServiceType serviceType, const std::string &eventId)
{
    std::vector<std::string> parts;
    boost::algorithm::split(parts, id, boost::is_any_of("."), boost::token_compress_on);

    if (3 != parts.size())
        return false;

    if ((ServiceTypeWatcher == serviceType || ServiceTypePublisher == serviceType) && parts[0] != PublisherWatcherPrefix)
    {
        return false;
    }

    if ((ServiceTypeWorker == serviceType || ServiceTypeDealer == serviceType) && parts[0] != DealerWorkerPrefix)
    {
        return false;
    }

    if (eventId != parts[1])
    {
        return false;
    }

    std::string hextokens = parts[2];
    parts.clear();

    boost::algorithm::split(parts, hextokens, boost::is_any_of("-"), boost::token_compress_on);
    if (2 != parts.size())
        return false;


    return (validateIdHexComponent(parts[0]) && validateIdHexComponent(parts[1]));
}

bool validateId(const std::string &id, ServiceType serviceType)
{
    std::vector<std::string> parts;
    boost::algorithm::split(parts, id, boost::is_any_of("."), boost::token_compress_on);

    if (2 > parts.size())
        return false;

    if ((ServiceTypeWatcher == serviceType || ServiceTypePublisher == serviceType) && parts[0] != PublisherWatcherPrefix)
    {
        return false;
    }

    if ((ServiceTypeWorker == serviceType || ServiceTypeDealer == serviceType) && parts[0] != DealerWorkerPrefix)
    {
        return false;
    }

    return true;
}

bool validateIdHexComponent(const std::string &hex)
{
    if (hex.size() != 4)
        return false;

    if (hex.find_first_not_of("0123456789ABCDEF") != std::string::npos)
    {
        return false;
    }

    return true;
}
