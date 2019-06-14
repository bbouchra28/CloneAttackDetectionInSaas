/*
 * xedApp.cc
 *
 *  Created on: Jun 12, 2019
 *      Author: bouchra
 */
#include "xedApp.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/udpapp/UdpBasicApp.h"
#include "inet/common/TagBase_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include <iterator>

using namespace std;
using namespace inet;
Define_Module(xedApp);

void xedApp::initialize(int stage)
{
    myApp::initialize(stage);
}

void xedApp::handleSelfMessage(cMessage *msg)
{
    if (msg == event)
    {
        /*--------------------------------------------------------------------------------------*/
        /********************************* Generate HELLO Message *******************************/
        /*--------------------------------------------------------------------------------------*/
        auto packet = generateHelloPacket();
        send(packet, "socketOut");
        packet = nullptr;
        delete packet;
        scheduleAt(simTime()+helloInterval+broadcastDelay->doubleValue(), event);
    }
    else
    {
        for (auto i : neighbors)
        {
            /*--------------------------------------------------------------------------------------*/
            /***************************** Generate and send XED Packet *****************************/
            /*--------------------------------------------------------------------------------------*/
            auto xed = makeShared<XedMsg>();
            xed->setChunkLength(b(128));
            xed->setSrcAddress(source);
            xed->setId(nodeID);
            xed->setDstAddress(i.adr);
            double random = generateRandom();
            xed->setRandom(random);
            XED *item_xed = new XED(source, i.adr, random);
            ls.push_back(*item_xed);
            auto packet_xed = new Packet("XED", xed);
            packet_xed->addTagIfAbsent<L3AddressReq>()->setDestAddress(i.adr);
            packet_xed->addTagIfAbsent<L3AddressReq>()->setSrcAddress(source);
            packet_xed->addTagIfAbsent<InterfaceReq>()->setInterfaceId(interface80211ptr->getInterfaceId());
            packet_xed->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::manet);
            packet_xed->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            send(packet_xed, "socketOut");
            packet_xed = nullptr;
            xed = nullptr;
            delete packet_xed;
            delete item_xed;
        }
    }
}

void xedApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        handleSelfMessage(msg);
    }
    else if (check_and_cast<Packet *>(msg)->getTag<PacketProtocolTag>()->getProtocol() == &Protocol::manet)
    {
        if (strcmp(check_and_cast<Packet *>(msg)->getName(),"Hello") == 0)
        {
            EV << "Received Hello message " << endl;
            auto recHello = staticPtrCast<HelloMsg>(check_and_cast<Packet *>(msg)->peekData<HelloMsg>()->dupShared());
            if (msg->arrivedOn("socketIn"))
            {
                bubble("Received hello message");
                Ipv4Address source = interface80211ptr->getProtocolData<Ipv4InterfaceData>()->getIPAddress();
                Ipv4Address src;
                int srcID;
                unsigned int msgsequencenumber;
                int numHops;
                Ipv4Address next;

                src = recHello->getSrcAddress();
                srcID = recHello->getId();
                msgsequencenumber = recHello->getSequencenumber();
                next = recHello->getNextAddress();
                numHops = recHello->getHopdistance();
                NEIGHBOR n;
                n.adr = src;
                n.id = srcID;

                if (src == source)
                {
                    EV_INFO << "Hello msg dropped. This message returned to the original creator.\n";
                    delete msg;
                    return;
                }
                else
                {
                    neighbors.push_back(n);
                    EV << "I am your neighbor(IPAddress, ID) = (" << src.str() << ", " << srcID << ")" << endl;
                    delete msg;
                }
                double waitTime = intuniform(1, 50);
                waitTime = waitTime/100;
                waitTime = SIMTIME_DBL(simTime())+waitTime;
                event2 = new cMessage("event2");
                scheduleAt(waitTime, event2);
             }

        }
        else if (strcmp(check_and_cast<Packet *>(msg)->getName(),"XED") == 0)
        {
            EV << "Received Xed message " << endl;
            auto recXed = staticPtrCast<XedMsg>(check_and_cast<Packet *>(msg)->peekData<XedMsg>()->dupShared());
            Ipv4Address srcAddress;
            Ipv4Address dstAddress;
            double random;
            srcAddress = recXed->getSrcAddress();
            dstAddress = recXed->getDstAddress();
            random = recXed->getRandom();
            XED *recitem = new XED(srcAddress, dstAddress, random);
            lr.push_back(*recitem);
            delete recitem;
            if (grandparent->getModuleByPath("Drones.clone") == nullptr)
                generateClone();
        }
    }
    else
        throw cRuntimeError("Message arrived on unknown gate %s", msg->getArrivalGate()->getName());
}

double xedApp::generateRandom()
{
    double lower_bound = 10000;
    double upper_bound = 100000;
    uniform_real_distribution<double> unif(lower_bound,upper_bound);
    default_random_engine re;
    double a_random_double = unif(re);
    return a_random_double;
}
