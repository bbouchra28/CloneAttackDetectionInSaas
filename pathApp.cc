/*
 * pathApp.cc
 *
 *  Created on: May 27, 2019
 *      Author: bouchra
 */

#include "pathApp.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/udpapp/UdpBasicApp.h"
#include "inet/common/TagBase_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include <iterator>

Define_Module(pathApp);

void pathApp::initialize(int stage)
{
    myApp::initialize(stage);
}

void pathApp::handleSelfMessage(cMessage *msg)
{
    if (msg == event)
    {
        /*--------------------------------------------------------------------------------------*/
        /**************************** Generate and send HELLO Packet ****************************/
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
            /***************************** Generate and send PATH Packet ****************************/
            /*--------------------------------------------------------------------------------------*/
            auto p = makeShared<PathMsg>();
            p->setChunkLength(b(128));
            p->setSrcAddress(source);
            p->setId(int(parent->par("myID")));
            p->setDstAddress(i.adr);
            p->setPath(path.str().c_str());

            auto packet_path = new Packet("PATH", p);
            packet_path->addTagIfAbsent<L3AddressReq>()->setDestAddress(i.adr);
            packet_path->addTagIfAbsent<L3AddressReq>()->setSrcAddress(source);
            packet_path->addTagIfAbsent<InterfaceReq>()->setInterfaceId(interface80211ptr->getInterfaceId());
            packet_path->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::manet);
            packet_path->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            send(packet_path, "socketOut");
            packet_path = nullptr;
            p = nullptr;
            delete packet_path;
        }
    }
}

void pathApp::handleMessageWhenUp(cMessage *msg)
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
                    if (n.id != (int)parent->par("myID"))
                    {
                        neighbors.push_back(n);
                        /*----PATH----*/
                        if (path.str() == "")
                        {
                            path << "(" << srcID << "), " << simTime() << ")";
                        }
                        else
                        {
                            path << ", (" << srcID << "), " << simTime() << ")";
                        }


                        EV << "I am your neighbor(IPAddress, ID) = (" << src.str() << ", " << srcID << ")" << endl;
                    }

                    delete msg;
                }
                double waitTime = intuniform(1, 50);
                waitTime = waitTime/100;
                waitTime = SIMTIME_DBL(simTime())+waitTime;
                if (event2 != nullptr) {
                   cancelAndDelete (event2);
                }
                event2 = new cMessage("event2");
                scheduleAt(waitTime, event2);
             }
        }
        else if (strcmp(check_and_cast<Packet *>(msg)->getName(),"PATH") == 0)
        {
            EV << "Received PATH message " << endl;
            auto recPath = staticPtrCast<PathMsg>(check_and_cast<Packet *>(msg)->peekData<PathMsg>()->dupShared());
            Ipv4Address srcAddress;
            Ipv4Address dstAddress;
            int srcID;
            const char *recp;
            srcAddress = recPath->getSrcAddress();
            dstAddress = recPath->getDstAddress();
            srcID = recPath->getId();
            recp = recPath->getPath();
            string rp (recp);
            EV << "I am node " << nodeID << " I received this path " << recp << " from node " << srcID << endl;
            PATH_ITEM item;
            hash<string> h;
            item.ID = srcID;
            item.length = rp.size();
            item.path = h(rp);
            auto It = find_if(received_path.begin(), received_path.end(), [=] (PATH_ITEM const& i){return (i.ID == srcID);});
            bool found = (It != received_path.end());
            if (!found)
            {
                EV << " I am node " << myID << " I don't have a path for node " << srcID << endl;
                received_path.push_back(item);
            }
            else
            {
                PATH_ITEM &x = *It;
                EV << "I am node " << myID << " I already have a path for node " << srcID << endl;
                string rp_base = rp.substr(0, x.length);
                if (x.path != h(rp_base))
                {
                    EV << "Actual path for node " << srcID << " is not equal to received path" << h(rp_base) << " != " << x.path << endl;
                    EV << "Clone attack detection : Node " << srcID << " has been cloned " << endl;
                    detected = 1;
                    duration = simTime();
                    endSimulation();
                }
                /*string p1 = string(x.path);
                string p2 = string(item.path);
                int size = (p1.size() < p2.size()) ? p1.size() : p2.size();
                if (p1.compare(0, size, p2.substr(0, size)) != 0)
                {
                    EV << "Actual path for node " << srcID << " is not equal to received path" << endl;
                    EV << "Clone attack detection : Node " << srcID << " has been cloned " << endl;
                    endSimulation();
                }*/

            }
            if (grandparent->getModuleByPath("Drones.clone") == nullptr)
                generateClone();
            delete msg;
        }
    }
    else
        throw cRuntimeError("Message arrived on unknown gate %s", msg->getArrivalGate()->getName());
}
