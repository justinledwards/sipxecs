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

class StateQueueConnectionTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(StateQueueConnectionTest);
//    CPPUNIT_TEST(consumeOKTest);
    CPPUNIT_TEST_SUITE_END();

    struct Packet
    {
      std::string body;
      std::string data;
      unsigned long dataLen;
    };

    class StateQueueAgentSimple : public StateQueueAgent
    {
    public:
      StateQueueAgentSimple(const std::string& agentId, ServiceOptions& options) : StateQueueAgent(agentId,options) {}

      ~StateQueueAgentSimple() {}

      void onIncomingRequest(StateQueueConnection& conn, const char* bytes, std::size_t bytes_transferred)
      {
        Packet expectedPacket = _packetStreamVec[_packetStreamPos];

        if (_packetStreamPos >= _packetStreamVec.size())
        {
          CPPUNIT_ASSERT(false);
          return;
        }

        CPPUNIT_ASSERT(bytes_transferred == expectedPacket.dataLen);
        CPPUNIT_ASSERT(0 == strncmp(bytes, expectedPacket.data.c_str(), expectedPacket.dataLen));
        _packetStreamPos++;
      }

      void expectRequests(std::vector<struct Packet>& packetStreamVec)
      {
        _packetStreamVec = packetStreamVec;
        _packetStreamPos = 0;
      }

      void clearExpected()
      {
        _packetStreamVec.clear();
        _packetStreamPos = 0;
      }

      std::vector<struct Packet> _packetStreamVec;
      unsigned int _packetStreamPos;
    };

    enum SplitWhere
    {
      SplitAfterVersionOneBit = 0,
      SplitAfterVersion,
      SplitAfterKeyOneBit,
      SplitAfterKey,
      SplitAfterLenOneBit,
      SplitAfterLenRandomBit,
      SplitAfterLen,
      SplitAfterDataOneBit,
      SplitAfterDataRandomBit,
      SplitAfterData,
      SplitPacketAllBits,   // couple of read required for a packet splitted after all the criteareas above
      SplitStreamPerPacket,  // only one read required for a packet (packet is complete)
      SplitWhereNum,
    };

public:

    void genRandom(char *s, const int len)
    {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        for (int i = 0; i < len; ++i) {
            s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }

        s[len] = 0;
    }

    void generatePacket(struct Packet& packet)
    {
      static char randCStr[1200];
      static bool hasRand = false;

      if (!hasRand)
      {
        genRandom(randCStr, 1200);
        hasRand = true;
      }

      short version = 1;
      short key = SQA_KEY_DEFAULT;

      packet.data = std::string(randCStr, packet.dataLen-1);

      std::stringstream strm;
      strm.write((char*)(&version), sizeof(version));
      strm.write((char*)(&key), sizeof(key));
      strm.write((char*)(&packet.dataLen), sizeof(packet.dataLen));
      strm << packet.data << "_";

      packet.body = strm.str();
    }

    void generatePacketStream(std::vector<struct Packet>& packetStreamVec, unsigned int streamLen)
    {
      int idx=0;
      unsigned int crtStreamLen = 0;

      while (crtStreamLen < streamLen)
      {
        struct Packet packet;
        if (0 == idx%2)
        {
          packet.dataLen = 10/*00*/;
        }
        else if (0 == idx%3)
        {
          packet.dataLen = 12/*00*/;
        }
        else
        {
          packet.dataLen = 6/*00*/;
        }

        generatePacket(packet);

        packetStreamVec.push_back(packet);
        crtStreamLen += packet.body.size();
        idx++;
      }
    }

    void sendSplittedPacketStream(StateQueueConnection* conn, std::vector<struct Packet>& packetStreamVec, int splitKind, unsigned int maxSendSize)
    {
      std::string buffer;
      std::cout << "      " <<  packetStreamVec.size() << "        " <<std::endl;
      for (unsigned int i = 0; i < packetStreamVec.size(); i++)
      {

        if (splitKind == SplitPacketAllBits)
        {
          memcpy(conn->_buffer.data(), packetStreamVec[i].body.data(), packetStreamVec[i].body.size());
          CPPUNIT_ASSERT(true == conn->consume(packetStreamVec[i].body.size()));
          continue;
        }
        else if (splitKind == SplitStreamPerPacket)
        {
          buffer =  packetStreamVec[i].body.substr(0, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
//          CPPUNIT_ASSERT(true == conn->consume(1));
          bool ret = conn->consume(buffer.size());
          std::cout << i << " 1 " << buffer.size() << " " << buffer << std::endl <<std::endl;
          if (false == ret)
          {
//            std::cout << i << " 1 " << buffer.size() << " " << buffer << std::endl <<std::endl;
            exit(1);
            CPPUNIT_ASSERT(false);
          }

          buffer =  packetStreamVec[i].body.substr(1, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
      //    CPPUNIT_ASSERT(true == conn->consume(1));
          ret = conn->consume(buffer.size());
          std::cout << i << " 2 " << buffer.size() << " " << buffer << std::endl <<std::endl;
          if (false == ret)
          {
//            std::cout << i << " 2 " << buffer.size() << " " << buffer << std::endl <<std::endl;
            exit(1);
            CPPUNIT_ASSERT(false);
          }

          buffer =  packetStreamVec[i].body.substr(2, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
        // CPPUNIT_ASSERT(true == conn->consume(1));
          ret = conn->consume(buffer.size());
          std::cout << i << " 3 " << buffer.size() << " " << buffer << std::endl <<std::endl;
          if (false == ret)
          {
//            std::cout << i << " 3 " << buffer.size() << " " << buffer << std::endl <<std::endl;
            exit(1);
            CPPUNIT_ASSERT(false);
          }

          buffer =  packetStreamVec[i].body.substr(3, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
         // CPPUNIT_ASSERT(true == conn->consume(1));
          ret = conn->consume(buffer.size());
          std::cout << i << " 4 " << buffer.size() << " " << buffer << std::endl <<std::endl;
          if (false == ret)
          {
//            std::cout << i << " 4 " << buffer.size() << " " << buffer << std::endl <<std::endl;
            exit(1);
            CPPUNIT_ASSERT(false);
          }

          buffer =  packetStreamVec[i].body.substr(4, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
      //    CPPUNIT_ASSERT(true == conn->consume(1));
          ret = conn->consume(buffer.size());
          std::cout << i << " 5 " << buffer.size() << " " << buffer << std::endl <<std::endl;
          if (false == ret)
          {
//            std::cout << i << " 5 " << buffer.size() << " " << buffer << std::endl <<std::endl;
            exit(1);
            CPPUNIT_ASSERT(false);
          }

          buffer =  packetStreamVec[i].body.substr(5, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());

//          CPPUNIT_ASSERT(true == conn->consume(7));
        ret = conn->consume(buffer.size());
        std::cout << i << " 6 " << buffer.size() << " " << buffer << std::endl <<std::endl;
        if (false == ret)
        {
          //std::cout << i << " 6 " << buffer.size() << " " << buffer << std::endl <<std::endl;
          exit(1);
          CPPUNIT_ASSERT(false);
        }


          buffer =  packetStreamVec[i].body.substr(6, 6);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
          //          CPPUNIT_ASSERT(true == conn->consume(7));
                  ret = conn->consume(buffer.size());
                  std::cout << i << " 7 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                  if (false == ret)
                  {
                    //std::cout << i << " 6 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                    exit(1);
                    CPPUNIT_ASSERT(false);
                  }


          buffer =  packetStreamVec[i].body.substr(12, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
          //          CPPUNIT_ASSERT(true == conn->consume(7));
                  ret = conn->consume(buffer.size());
                  std::cout << i << " 8 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                  if (false == ret)
                  {
                    //std::cout << i << " 6 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                    exit(1);
                    CPPUNIT_ASSERT(false);
                  }


          buffer =  packetStreamVec[i].body.substr(13, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
          //          CPPUNIT_ASSERT(true == conn->consume(7));
                  ret = conn->consume(buffer.size());
                  std::cout << i << " 9 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                  if (false == ret)
                  {
                    //std::cout << i << " 6 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                    exit(1);
                    CPPUNIT_ASSERT(false);
                  }


          buffer =  packetStreamVec[i].body.substr(14, packetStreamVec[i].dataLen - 3);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
          //          CPPUNIT_ASSERT(true == conn->consume(7));
                  ret = conn->consume(buffer.size());
                  std::cout << i << " 10 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                  if (false == ret)
                  {
                    //std::cout << i << " 6 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                    exit(1);
                    CPPUNIT_ASSERT(false);
                  }

                  if (i > 1427)
                  {
                    std::cout << "here " << std::endl;
                    i--;
                    i++;
                    buffer =  "d";
                    std::cout << buffer << std::endl;
                  }

          buffer =  packetStreamVec[i].body.substr(12 + packetStreamVec[i].dataLen - 1, 1);
          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
          //          CPPUNIT_ASSERT(true == conn->consume(7));
                  ret = conn->consume(buffer.size());
                  std::cout << i << " 11 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                  if (false == ret)
                  {
                    //std::cout << i << " 6 " << buffer.size() << " " << buffer << std::endl <<std::endl;
                    exit(1);
                    CPPUNIT_ASSERT(false);
                  }



          continue;
        }

        if (buffer.size() > maxSendSize)
        {
          std::string reminder;
          switch (splitKind)
          {
          case SplitAfterVersionOneBit:
            buffer +=  packetStreamVec[i].body.substr(0, 1);
            reminder =  packetStreamVec[i].body.substr(1);
            break;
          case SplitAfterVersion:
            buffer +=  packetStreamVec[i].body.substr(0, 2);
            reminder =  packetStreamVec[i].body.substr(2);
            break;
          case SplitAfterKeyOneBit:
            buffer +=  packetStreamVec[i].body.substr(0, 3);
            reminder =  packetStreamVec[i].body.substr(3);
            break;
          case SplitAfterKey:
            buffer +=  packetStreamVec[i].body.substr(0, 4);
            reminder =  packetStreamVec[i].body.substr(4);
            break;
          case SplitAfterLenOneBit:
            buffer +=  packetStreamVec[i].body.substr(0, 5);
            reminder =  packetStreamVec[i].body.substr(5);
            break;
          case SplitAfterLenRandomBit:
            buffer +=  packetStreamVec[i].body.substr(0, 6);
            reminder =  packetStreamVec[i].body.substr(6);
            break;
          case SplitAfterLen:
            buffer +=  packetStreamVec[i].body.substr(0, 12);
            reminder =  packetStreamVec[i].body.substr(12);
            break;
          case SplitAfterDataOneBit:
            buffer +=  packetStreamVec[i].body.substr(0, 13);
            reminder =  packetStreamVec[i].body.substr(13);
            break;
          case SplitAfterDataRandomBit:
            buffer +=  packetStreamVec[i].body.substr(0, 14);
            reminder =  packetStreamVec[i].body.substr(14);
            break;
          case SplitAfterData:
            buffer +=  packetStreamVec[i].body.substr(0, 12 + packetStreamVec[i].dataLen - 1);
            reminder =  packetStreamVec[i].body.substr(12 + packetStreamVec[i].dataLen - 1);
            break;
          default:
            break;
          }

          memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
          CPPUNIT_ASSERT(true == conn->consume(buffer.size()));
          buffer = reminder;
        }
        else
        {
          buffer += packetStreamVec[i].body;
        }
      }

      if (buffer.size() > 0)
      {
        memcpy(conn->_buffer.data(), buffer.data(), buffer.size());
        bool ret = conn->consume(buffer.size());
        CPPUNIT_ASSERT(ret);
      }
    }

    void consumeOKTest()
    {
      _agentsData.clear();
      _util.generateSQAAgentData(_agentsData, 1, false);

      StateQueueAgentSimple agentSimple("agent simple", *_agentsData[0]->service);

      StateQueueConnection* conn = new StateQueueConnection(agentSimple._ioService, agentSimple);


      std::vector<struct Packet> packetStreamVec;
      unsigned int streamLen=30000/*00*/;
      generatePacketStream(packetStreamVec, streamLen);

      agentSimple.expectRequests(packetStreamVec);

      int splitKind = 0;
      for (splitKind = 11; splitKind < 12/*SplitWhereNum*/; splitKind++)
      {
        agentSimple.expectRequests(packetStreamVec);
        sendSplittedPacketStream(conn, packetStreamVec, splitKind, 600/*00*/);

        agentSimple.expectRequests(packetStreamVec);
        sendSplittedPacketStream(conn, packetStreamVec, splitKind, 200/*00*/);

        agentSimple.expectRequests(packetStreamVec);
        sendSplittedPacketStream(conn, packetStreamVec, splitKind, 400/*00*/);
      }
    }

    std::vector<SQAAgentData::Ptr> _agentsData;
    SQAAgentUtil _util;
};

CPPUNIT_TEST_SUITE_REGISTRATION(StateQueueConnectionTest);
