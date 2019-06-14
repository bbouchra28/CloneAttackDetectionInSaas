/*
 * mtlsdApp.cc
 *
 *  Created on: Jun 12, 2019
 *      Author: bouchra
 */

#include "mtlsdApp.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/udpapp/UdpBasicApp.h"
#include "inet/common/TagBase_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include <iterator>

Define_Module(mtlsdApp);

void mtlsdApp::initialize(int stage)
{
    myApp::initialize(stage);
    IMobility *mobility = check_and_cast<IMobility *>(parent->getSubmodule("mobility"));
    maxSpeed = mobility->getMaxSpeed();
}

void mtlsdApp::handleSelfMessage(cMessage *msg)
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
    else if (msg == event2)
    {
        for (auto i : neighbors)
        {
            /*--------------------------------------------------------------------------------------*/
            /***************************** Generate and send MTLSD Packet ***************************/
            /*--------------------------------------------------------------------------------------*/
            auto mtlsd = makeShared<MtlsdMsg>();
            mtlsd->setChunkLength(b(128));
            mtlsd->setSrcAddress(source);
            mtlsd->setId(int(parent->par("myID")));
            mtlsd->setDstAddress(i.adr);
            simtime_t time = simTime();
            mtlsd->setTime(time);
            IMobility *mobility = check_and_cast<IMobility *>(parent->getSubmodule("mobility"));
            Coord position = mobility->getCurrentPosition();
            string pos = position.str();
            EV_DEBUG << " This mobility pos in string " << pos << endl;
            const char *myPos = pos.c_str();
            mtlsd->setPosition(myPos);
            EV_DEBUG << "getting what I set position " << string(mtlsd->getPosition()) << endl;
            auto packet_mtlsd = new Packet("MTLSD", mtlsd);
            packet_mtlsd->addTagIfAbsent<L3AddressReq>()->setDestAddress(i.adr);
            packet_mtlsd->addTagIfAbsent<L3AddressReq>()->setSrcAddress(source);
            packet_mtlsd->addTagIfAbsent<InterfaceReq>()->setInterfaceId(interface80211ptr->getInterfaceId());
            packet_mtlsd->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::manet);
            packet_mtlsd->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
            send(packet_mtlsd, "socketOut");
            packet_mtlsd = nullptr;
            mtlsd = nullptr;
            delete packet_mtlsd;
        }
    }
    else
    {
        /*for (auto node=mtlsd_file.begin(); node!= mtlsd_file.end(); ++node)
        {
            if ((int(parent->par("myID"))) < (node->ID))
            {
                if (!node->q.empty())
                {
                    EV_DEBUG << "For node " << node->ID << " The queue is not empty, an MTLSDFILE packet will be sent " << endl;
                    for (auto i : neighbors)
                    {
                        auto mtlsdfile = makeShared<MtlsdFile>();
                        mtlsdfile->setChunkLength(b(128));
                        mtlsdfile->setSrcAddress(source);
                        mtlsdfile->setId(int(parent->par("myID")));;
                        mtlsdfile->setDstAddress(i.adr);
                        MTLSD e1, e2, e3;
                        EV_DEBUG << "My own file is of size " << node->q.size() << endl;
                        if(node->q.size() == 1)
                        {
                            e1 = node->q.front();
                            node->q.pop_front();
                            mtlsdfile->setTime1(e1.time);
                            mtlsdfile->setPosition1((e1.position).c_str());
                            mtlsdfile->setTime2(SIMTIME_ZERO);
                            mtlsdfile->setPosition2(nullptr);
                            mtlsdfile->setTime3(SIMTIME_ZERO);
                            mtlsdfile->setPosition3(nullptr);
                            EV_DEBUG << " I set the first time-location (" << e1.position << ", " << e1.time << ")" << endl;
                            node->q.push_back(e1);
                        }
                        else if (node->q.size() == 2 )
                        {
                            e1 = node->q.front();
                            node->q.pop_front();
                            mtlsdfile->setTime1(e1.time);
                            mtlsdfile->setPosition1((e1.position).c_str());
                            e2 = node->q.front();
                            node->q.pop_front();
                            mtlsdfile->setTime2(e2.time);
                            mtlsdfile->setPosition2((e2.position).c_str());
                            mtlsdfile->setTime3(SIMTIME_ZERO);
                            mtlsdfile->setPosition3(nullptr);
                            EV_DEBUG << " I set the first time-location (" << e1.position << ", " << e1.time << ")" << endl;
                            EV_DEBUG << " I set the the second time-location (" << e2.position << ", " << e2.time << ")" << endl;
                            node->q.push_back(e1);
                            node->q.push_back(e2);
                        }
                        else if (node->q.size() == 3 )
                        {
                            e1 = node->q.front();
                            node->q.pop_front();
                            mtlsdfile->setTime1(e1.time);
                            mtlsdfile->setPosition1((e1.position).c_str());
                            e2 = node->q.front();
                            node->q.pop_front();
                            e3 = node->q.front();
                            node->q.pop_front();
                            mtlsdfile->setTime2(e2.time);
                            mtlsdfile->setPosition2((e2.position).c_str());
                            mtlsdfile->setTime3(e3.time);
                            mtlsdfile->setPosition3((e3.position).c_str());
                            EV_DEBUG << " I set the first time-location (" << e1.position << ", " << e1.time << ")" << endl;
                            EV_DEBUG << " I set the the second time-location (" << e2.position << ", " << e2.time << ")" << endl;
                            EV_DEBUG << " I set the third time-location (" << e3.position << ", " << e3.time << ")" << endl;
                            node->q.push_back(e1);
                            node->q.push_back(e2);
                            node->q.push_back(e3);
                        }
                        auto packet_mtlsdfile = new Packet("MTLSDFILE", mtlsdfile);
                        packet_mtlsdfile->addTagIfAbsent<L3AddressReq>()->setDestAddress(i.adr);
                        packet_mtlsdfile->addTagIfAbsent<L3AddressReq>()->setSrcAddress(source);
                        packet_mtlsdfile->addTagIfAbsent<InterfaceReq>()->setInterfaceId(interface80211ptr->getInterfaceId());
                        packet_mtlsdfile->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::manet);
                        packet_mtlsdfile->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
                        send(packet_mtlsdfile, "socketOut");
                        EV_DEBUG << " I have sent the MTLSDFILE Packet " << endl;
                        packet_mtlsdfile = nullptr;
                        mtlsdfile = nullptr;
                        delete packet_mtlsdfile;
                    }
                }
            }
        }*/
    }
}

void mtlsdApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        handleSelfMessage(msg);
    }
    else if (check_and_cast<Packet *>(msg)->getTag<PacketProtocolTag>()->getProtocol() == &Protocol::manet)
    {
        if (strcmp(check_and_cast<Packet *>(msg)->getName(),"Hello") == 0)
        {
            EV_DEBUG << "Received Hello message " << endl;
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
                    EV_DEBUG << "Hello msg dropped. This message returned to the original creator.\n";
                    delete msg;
                    return;
                }
                else
                {
                    neighbors.push_back(n);
                    EV_DEBUG << "I am your neighbor(IPAddress, ID) = (" << src.str() << ", " << srcID << ")" << endl;
                    delete msg;
                }
                double waitTime = intuniform(1, 50);
                waitTime = waitTime/100;
                waitTime = SIMTIME_DBL(simTime())+waitTime;
                event2 = new cMessage("event2");
                scheduleAt(waitTime + uniform(0.0, par("maxVariance").doubleValue()), event2);
             }

        }
        else if (strcmp(check_and_cast<Packet *>(msg)->getName(),"MTLSD") == 0)
        {
            EV_DEBUG << "Received Mtlsd message " << endl;
            auto recMtlsd = staticPtrCast<MtlsdMsg>(check_and_cast<Packet *>(msg)->peekData<MtlsdMsg>()->dupShared());
            Ipv4Address srcAddress;
            Ipv4Address dstAddress;
            const char *position;
            simtime_t time;
            int srcID;
            srcAddress = recMtlsd->getSrcAddress();
            dstAddress = recMtlsd->getDstAddress();
            position = recMtlsd->getPosition();
            time = recMtlsd->getTime();
            srcID = recMtlsd->getId();
            EV_DEBUG << "I am node " << myID << " I received this time-location claim (" << srcID << ", " << position << ", " << time << ") from node " << srcID << endl;
            MTLSD recitem;
            recitem.originatorAddr = srcAddress;
            recitem.originatorId = srcID;
            recitem.destinationAddr = dstAddress;
            recitem.position = string(position);
            recitem.time = time;
            if (int(parent->par("myID")) != srcID)
            {
                auto node = find_if(mtlsd_file.begin(), mtlsd_file.end(), [=] (MTLSD_DATA const& i){return (i.ID == srcID);});
                bool found = (node != mtlsd_file.end());
                if (!found)
                {
                    MTLSD_DATA recdata;
                    recdata.ID = srcID;
                    recdata.q.push_back(recitem);
                    mtlsd_file.push_back(recdata);
                    EV_DEBUG << "For node " << srcID ;
                    for(auto claim=mtlsd_file.back().q.begin(); claim!=mtlsd_file.back().q.end();++claim)
                    {
                        EV_DEBUG << "(" << string(claim->position) << ", " << claim->time << ");" << endl;
                    }
                }
                else
                {
                    EV_DEBUG << "I already have data about the node " << node->ID << endl;
                    auto search = find_if(node->q.begin(), node->q.end(), [=] (MTLSD const& i){return ((i.originatorId == recitem.originatorId)
                            && (i.originatorAddr == recitem.originatorAddr) && (i.destinationAddr == recitem.destinationAddr) &&
                            (i.position == recitem.position) && (i.time == recitem.time));});
                    bool exist = (search != node->q.end());
                    if (!exist)
                    {
                        if (node->q.size() == 3)
                        {
                            EV_DEBUG << "I already have three time-location claim in the queue" << endl;
                            EV_DEBUG << "Here they are: ";
                            EV_DEBUG << "For node " << (*node).ID << endl;
                            for(auto fileclaim=(*node).q.begin(); fileclaim!=(*node).q.end();++fileclaim)
                                EV_DEBUG << "(" << string((*fileclaim).position) << ", " << (*fileclaim).time << ");" << endl;
                            EV_DEBUG << "I will delete the old one (" << node->q.front().position << ", " << node->q.front().time << ")" << endl;
                            node->q.pop_front();
                        }
                        node->q.push_back(recitem);
                        EV_DEBUG << "I have pushed this new one : (" << string(node->q.back().position) << ", " << node->q.back().time << ")" << endl;
                    }
                    EV_DEBUG << "Here they are all time-location claims in the queue : ";
                    for(auto fileclaims=node->q.begin(); fileclaims!=node->q.end();++fileclaims)
                    {
                        EV_DEBUG << "(" << string(fileclaims->position) << ", " << fileclaims->time << ");" << endl;
                    }
                }
            }
            if (grandparent->getModuleByPath("Drones.clone") == nullptr)
                generateClone();
            delete msg;
        }
        else if (strcmp(check_and_cast<Packet *>(msg)->getName(),"MTLSDFILE") == 0)
        {
            auto recMtlsd = staticPtrCast<MtlsdFile>(check_and_cast<Packet *>(msg)->peekData<MtlsdFile>()->dupShared());
            Ipv4Address srcAddress;
            Ipv4Address dstAddress;
            const char *position1;
            const char *position2;
            const char *position3;
            simtime_t time1, time2, time3;
            int srcID;
            srcID = recMtlsd->getId();
            srcAddress = recMtlsd->getSrcAddress();
            dstAddress = recMtlsd->getDstAddress();
            position1 = recMtlsd->getPosition1();
            time1 = recMtlsd->getTime1();
            EV_DEBUG << "Received Mtlsdfile message from node " << srcID << endl;
            MTLSD recitem1;
            recitem1.originatorAddr = srcAddress;
            recitem1.originatorId = srcID;
            recitem1.destinationAddr = dstAddress;
            recitem1.position = string(position1);
            recitem1.time = time1;
            mtlsdqueue.push_back(recitem1);
            EV_DEBUG << "First time-location claim (" << srcID << ", " << string(recMtlsd->getPosition1()) << ", " << recMtlsd->getTime1() << ")" << endl;
            EV_DEBUG << "Second time-location claim (" << srcID << ", " << string(recMtlsd->getPosition2()) << ", " << recMtlsd->getTime2() << ")" << endl;
            EV_DEBUG << "Third time-location claim (" << srcID << ", " << string(recMtlsd->getPosition3()) << ", " << recMtlsd->getTime3() << ")" << endl;
            if (recMtlsd->getPosition2() == nullptr)
            {
                position2 = recMtlsd->getPosition2();
                time2 = recMtlsd->getTime2();
                MTLSD recitem2;
                recitem2.originatorAddr = srcAddress;
                recitem2.originatorId = srcID;
                recitem2.destinationAddr = dstAddress;
                recitem2.position = string(position2);
                recitem2.time = time2;
                mtlsdqueue.push_back(recitem2);
                EV_DEBUG << "Second time-location claim (" << srcID << ", " << string(position2) << ", " << time2 << ")" << endl;
            }
            if (recMtlsd->getPosition3() == nullptr)
            {
                position3 = recMtlsd->getPosition3();
                time3 = recMtlsd->getTime3();
                MTLSD recitem3;
                recitem3.originatorAddr = srcAddress;
                recitem3.originatorId = srcID;
                recitem3.destinationAddr = dstAddress;
                recitem3.position = string(position3);
                recitem3.time = time3;
                mtlsdqueue.push_back(recitem3);
                EV_DEBUG << "Third time-location claim (" << srcID << ", " << string(position3) << ", " << time3 << ")" << endl;
            }

            srcID = recMtlsd->getId();

            for (auto i:mtlsd_file)
            {
                if (i.ID == srcID)
                {
                    EV_DEBUG <<  "Node for which we detect is " << srcID << " and " << i.ID << endl;
                    if (!mtlsdqueue.empty())
                        for(auto it=i.q.begin(); it!=i.q.end();++it)
                        {
                            EV_DEBUG << "size of received " << mtlsdqueue.size() << endl;
                            EV_DEBUG << "size of what i have " << i.q.size() << endl;
                            string p1 = string(mtlsdqueue.front().position);
                            simtime_t t1 = mtlsdqueue.front().time;
                            string p2 = string(it->position);
                            simtime_t t2 = it->time;

                            EV_DEBUG << "I have " << p2 << endl;
                            EV_DEBUG << "I received " << p1 << endl;
                            if (abs(int(p1 - p2)) > abs(int(t1 - t2)) * maxSpeed)
                            EV_DEBUG << "Clone attack detection : Node " << srcID << " has been cloned " << endl;
                            //endSimulation();
                            mtlsdqueue.pop_front();
                        }
                }
            }
        }
    }
    else
        throw cRuntimeError("Message arrived on unknown gate %s", msg->getArrivalGate()->getName());
}


