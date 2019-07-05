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
    range = 250;
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
            double posX, posY, posZ;
            posX = position.getX();
            posY = position.getY();
            posZ = position.getZ();
            mtlsd->setPositionX(posX);
            mtlsd->setPositionY(posY);
            mtlsd->setPositionZ(posZ);
            for (auto i : neighbors)
            {
                mtlsd->insertWitness_nodes(i.id);
            }
            EV_DEBUG << "getting what I set position (" << mtlsd->getPositionX() << ", " << mtlsd->getPositionY() << ", " << mtlsd->getPositionZ() << ")" << endl;
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
                    if (n.id != (int)parent->par("myID"))
                    {
                        neighbors.push_back(n);
                        nb.insert(n.id);
                        EV_DEBUG << "I am your neighbor(IPAddress, ID) = (" << src.str() << ", " << srcID << ")" << endl;
                    }
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
            double X, Y, Z;
            simtime_t time;
            int srcID;
            set<int> witness;
            srcAddress = recMtlsd->getSrcAddress();
            dstAddress = recMtlsd->getDstAddress();
            X = recMtlsd->getPositionX();
            Y = recMtlsd->getPositionY();
            Z = recMtlsd->getPositionZ();
            time = recMtlsd->getTime();
            srcID = recMtlsd->getId();
            int size = recMtlsd->getWitness_nodesArraySize();
            for (int k = 0; k < size; k++)
            {
                witness.insert(recMtlsd->getWitness_nodes(k));
            }
            if ( srcID != ((int)parent->par("myID")))
            {
                EV_DEBUG << "I am node " << myID << " I received this time-location claim (" << srcID << ", (" << X << ", " << Y << ", " << Z << "), " << time << ") from node " << srcID << endl;
                MTLSD recitem;
                recitem.originatorAddr = srcAddress;
                recitem.originatorId = srcID;
                recitem.destinationAddr = dstAddress;
                recitem.positionx = X;
                recitem.positiony = Y;
                recitem.positionz = Z;
                recitem.time = time;
                double diffT = abs((simTime() - time).dbl());
                IMobility *mobility = check_and_cast<IMobility *>(parent->getSubmodule("mobility"));
                Coord position = mobility->getCurrentPosition();
                double posX, posY, posZ;
                posX = position.getX();
                posY = position.getY();
                posZ = position.getZ();
                double diffx = abs(int(X - posX));
                double diffy = abs(int(Y - posY));
                double diffz = abs(int(Z - posZ));
                if (int(parent->par("myID")) != srcID)
                {
                    auto node = find_if(mtlsd_file.begin(), mtlsd_file.end(), [=] (MTLSD_DATA const& i){return (i.ID == srcID);});
                    bool found = (node != mtlsd_file.end());
                    if (!found)
                    {
                        EV << "Not found" << endl;
                        MTLSD_DATA recdata;
                        recdata.ID = srcID;
                        if ((diffT < epsilon) && ((diffx < range) && (diffy < range) && (diffz < range)))
                        {
                            recdata.q.push_back(recitem);
                            mtlsd_file.push_back(recdata);
                            EV << "This is what I pushed " << mtlsd_file.back().q.back().positiony << endl;
                        }
                    }
                    else
                    {
                        EV_DEBUG << "I already have data about the node " << node->ID << endl;
                        auto search = find_if(node->q.begin(), node->q.end(), [=] (MTLSD const& i){return ((i.originatorId == recitem.originatorId)
                                && (i.originatorAddr == recitem.originatorAddr) && (i.destinationAddr == recitem.destinationAddr) &&
                                (i.positionx == recitem.positionx) && (i.positiony == recitem.positiony) && (i.positionz == recitem.positionz) && (i.time == recitem.time));});
                        bool exist = (search != node->q.end());
                        if (!exist)
                        {
                            if ((diffT < epsilon) && ((diffx < range) && (diffy < range) && (diffz < range)))
                            {
                                if (node->q.size() == 3)
                                {
                                    EV_DEBUG << "I already have three time-location claim in the queue" << endl;
                                    EV_DEBUG << "Here they are: ";
                                    EV_DEBUG << "For node " << (*node).ID << endl;
                                    for(auto fileclaim=(*node).q.begin(); fileclaim!=(*node).q.end();++fileclaim)
                                        EV_DEBUG << "((" << fileclaim->positionx << ", " << fileclaim->positiony << ", " << fileclaim->positionz << "), " << fileclaim->time << ");" << endl;
                                    EV_DEBUG << "I will delete the old one ((" << node->q.front().positionx << ", " << node->q.front().positiony << ", " << node->q.front().positionz << "), " << node->q.front().time << ")" << endl;
                                    node->q.pop_front();
                                }
                                node->q.push_back(recitem);
                                EV_DEBUG << "I have pushed this new one : ((" << node->q.back().positionx << ", " << node->q.back().positiony << ", " << node->q.back().positionz << "), " << node->q.back().time << ")" << endl;
                            }
                        }
                    }
                    EV_DEBUG << "Here they are all time-location claims in the queue : ";
                    for(auto fileclaims = mtlsd_file.begin(); fileclaims != mtlsd_file.end(); ++fileclaims)
                    {
                        EV << "For node " << fileclaims->ID << endl;
                        for(auto n = fileclaims->q.begin(); n != fileclaims->q.end(); ++n)
                            EV_DEBUG << "((" << n->positionx << ", " << n->positiony << ", " << n->positionz << "), " << n->time << ");" << endl;
                    }
                }
                EV_DEBUG << " Neighbors of node " << (int)parent->par("myID") << ": ";
                for (auto j:nb)
                {
                    EV_DEBUG << " " << j << " ";
                }
                EV << endl;
                EV_DEBUG << " Witnesses of node " << srcID << ": ";
                for (auto j:witness)
                {
                    EV_DEBUG << " " << j << " ";
                }
                EV << endl;
                set<int> out;
                set_intersection(nb.begin(), nb.end(), witness.begin(), witness.end(),
                                          inserter(out, out.begin()));
                EV_DEBUG << "Intersection :";
                for (auto k : out)
                {
                    EV_DEBUG << k << " ";
                }
                EV << endl;
                if (!out.empty())
                {
                    EV_DEBUG << " I am node " << (int)parent->par("myID") << ", i have shared witnesses with node " << srcID << endl;
                    EV_DEBUG << " These nodes are : ";
                    for (auto k : out)
                    {
                        EV_DEBUG << " " << k;
                        auto node = find_if(mtlsd_file.begin(), mtlsd_file.end(), [=] (MTLSD_DATA const& i){return (i.ID == k);});
                        bool found = (node != mtlsd_file.end());
                        if (found)
                        {
                            if ((int(parent->par("myID"))) < (srcID))
                            {
                                if (!node->q.empty())
                                {
                                    EV_DEBUG << "For node " << node->ID << " The queue is not empty, an MTLSDFILE packet will be sent " << endl;
                                    auto mtlsdfile = makeShared<MtlsdFile>();
                                    mtlsdfile->setChunkLength(b(128));
                                    mtlsdfile->setSrcAddress(source);
                                    mtlsdfile->setSrcId((int)parent->par("myID"));
                                    mtlsdfile->setClaimId(k);
                                    //mtlsdfile->setDstAddress(i.adr);
                                    list <MTLSD> file = node->q;
                                    MTLSD e1, e2, e3;
                                    EV_DEBUG << "My own file is of size " << node->q.size() << endl;
                                    if(node->q.size() == 1)
                                    {
                                        e1 = file.front();
                                        mtlsdfile->setTime1(e1.time);
                                        mtlsdfile->setPosition1X(e1.positionx);
                                        mtlsdfile->setPosition1Y(e1.positiony);
                                        mtlsdfile->setPosition1Z(e1.positionz);
                                        file.pop_front();
                                        mtlsdfile->setTime2(SIMTIME_ZERO);
                                        mtlsdfile->setPosition2X(0);
                                        mtlsdfile->setPosition2Y(0);
                                        mtlsdfile->setPosition2Z(0);
                                        mtlsdfile->setTime3(SIMTIME_ZERO);
                                        mtlsdfile->setPosition3X(0);
                                        mtlsdfile->setPosition3Y(0);
                                        mtlsdfile->setPosition3Z(0);
                                        EV_DEBUG << " I set the first time-location ((" << e1.positionx << ", " << e1.positiony << ", " << e1.positionz << "), "<< e1.time << ")" << endl;
                                    }
                                    else if (node->q.size() == 2 )
                                    {
                                        e1 = file.front();
                                        mtlsdfile->setTime1(e1.time);
                                        mtlsdfile->setPosition1X(e1.positionx);
                                        mtlsdfile->setPosition1Y(e1.positiony);
                                        mtlsdfile->setPosition1Z(e1.positionz);
                                        file.pop_front();
                                        e2 = file.front();
                                        mtlsdfile->setTime2(e2.time);
                                        mtlsdfile->setPosition2X(e2.positionx);
                                        mtlsdfile->setPosition2Y(e2.positiony);
                                        mtlsdfile->setPosition2Z(e2.positionz);
                                        file.pop_front();
                                        mtlsdfile->setTime3(SIMTIME_ZERO);
                                        mtlsdfile->setPosition3X(0);
                                        mtlsdfile->setPosition3Y(0);
                                        mtlsdfile->setPosition3Z(0);
                                        EV_DEBUG << " I set the first time-location ((" << e1.positionx << ", " << e1.positiony << ", " << e1.positionz << "), "<< e1.time << ")" << endl;
                                        EV_DEBUG << " I set the the second time-location ((" << e2.positionx << ", " << e2.positiony << ", " << e2.positionz << "), "<< e2.time << ")" << endl;
                                    }
                                    else if (node->q.size() == 3 )
                                    {
                                        e1 = file.front();
                                        mtlsdfile->setTime1(e1.time);
                                        mtlsdfile->setPosition1X(e1.positionx);
                                        mtlsdfile->setPosition1Y(e1.positiony);
                                        mtlsdfile->setPosition1Z(e1.positionz);
                                        file.pop_front();
                                        e2 = file.front();
                                        mtlsdfile->setTime2(e2.time);
                                        mtlsdfile->setPosition2X(e2.positionx);
                                        mtlsdfile->setPosition2Y(e2.positiony);
                                        mtlsdfile->setPosition2Z(e2.positionz);
                                        file.pop_front();
                                        e3 = file.front();
                                        mtlsdfile->setTime3(e3.time);
                                        mtlsdfile->setPosition3X(e3.positionx);
                                        mtlsdfile->setPosition3Y(e3.positiony);
                                        mtlsdfile->setPosition3Z(e3.positionz);
                                        file.pop_front();
                                        EV_DEBUG << " I set the first time-location ((" << e1.positionx << ", " << e1.positiony << ", " << e1.positionz << "), "<< e1.time << ")" << endl;
                                        EV_DEBUG << " I set the the second time-location ((" << e2.positionx << ", " << e2.positiony << ", " << e2.positionz << "), "<< e2.time << ")" << endl;
                                        EV_DEBUG << " I set the third time-location ((" << e3.positionx << ", " << e3.positiony << ", " << e3.positionz << "), "<< e3.time << ")" << endl;
                                    }
                                    auto packet_mtlsdfile = new Packet("MTLSDFILE", mtlsdfile);
                                    packet_mtlsdfile->addTagIfAbsent<L3AddressReq>()->setDestAddress(Ipv4Address(255, 255, 255, 255));
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
                    }
                }

                if (grandparent->getModuleByPath("Drones.clone") == nullptr)
                    generateClone();
                delete msg;
            }
        }
        else if (strcmp(check_and_cast<Packet *>(msg)->getName(),"MTLSDFILE") == 0)
        {
            auto recMtlsd = staticPtrCast<MtlsdFile>(check_and_cast<Packet *>(msg)->peekData<MtlsdFile>()->dupShared());
            Ipv4Address srcAddress;
            Ipv4Address dstAddress;
            double position1x, position1y, position1z;
            double position2x, position2y, position2z;
            double position3x, position3y, position3z;
            simtime_t time1, time2, time3;
            int srcID, claimID;
            srcID = recMtlsd->getSrcId();
            srcAddress = recMtlsd->getSrcAddress();
            dstAddress = recMtlsd->getDstAddress();
            position1x = recMtlsd->getPosition1X();
            position1y = recMtlsd->getPosition1Y();
            position1z = recMtlsd->getPosition1Z();
            time1 = recMtlsd->getTime1();
            claimID = recMtlsd->getClaimId();
            EV_DEBUG << "Received Mtlsdfile message from node " << srcID << endl;
            MTLSD recitem1;
            recitem1.originatorAddr = srcAddress;
            recitem1.originatorId = claimID;
            recitem1.destinationAddr = dstAddress;
            recitem1.positionx = position1x;
            recitem1.positiony = position1y;
            recitem1.positionz = position1z;
            recitem1.time = time1;

            mtlsdqueue.push_back(recitem1);
            EV_DEBUG << "First time-location claim (" << claimID << ", (" << recMtlsd->getPosition1X() << recMtlsd->getPosition1Y() << ", " << recMtlsd->getPosition1Z() << "), " << recMtlsd->getTime1() << ")" << endl;
            EV_DEBUG << "Second time-location claim (" << srcID << ", (" << recMtlsd->getPosition2X() << recMtlsd->getPosition2Y() << ", " << recMtlsd->getPosition2Z() << "), " << recMtlsd->getTime2() << ")" << endl;
            EV_DEBUG << "Third time-location claim (" << srcID << ", (" << recMtlsd->getPosition3X() << recMtlsd->getPosition3Y() << ", " << recMtlsd->getPosition3Z() << "), " << recMtlsd->getTime3() << ")" << endl;
            if ((recMtlsd->getPosition2X() != 0) && (recMtlsd->getPosition2Y() != 0) && (recMtlsd->getPosition2Z() != 0) && (recMtlsd->getTime2() != SIMTIME_ZERO))
            {
                position2x = recMtlsd->getPosition2X();
                position2y = recMtlsd->getPosition2Y();
                position2z = recMtlsd->getPosition2Z();
                time2 = recMtlsd->getTime2();
                MTLSD recitem2;
                recitem2.originatorAddr = srcAddress;
                recitem2.originatorId = claimID;
                recitem2.destinationAddr = dstAddress;
                recitem2.positionx = position2x;
                recitem2.positiony = position2y;
                recitem2.positionz = position2z;
                recitem2.time = time2;
                mtlsdqueue.push_back(recitem2);
                EV_DEBUG << "Second time-location claim (" << claimID << ",( " << position2x << ", " << position2y << ", " << position2z << "), " << time2 << ")" << endl;
            }
            if ((recMtlsd->getPosition3X() != 0) && (recMtlsd->getPosition3Y() != 0) && (recMtlsd->getPosition3Z() != 0) && (recMtlsd->getTime3() != SIMTIME_ZERO))
            {
                position3x = recMtlsd->getPosition3X();
                position3y = recMtlsd->getPosition3Y();
                position3z = recMtlsd->getPosition3Z();
                time3 = recMtlsd->getTime3();
                MTLSD recitem3;
                recitem3.originatorAddr = srcAddress;
                recitem3.originatorId = claimID;
                recitem3.destinationAddr = dstAddress;
                recitem3.positionx = position3x;
                recitem3.positiony = position3y;
                recitem3.positionz = position3z;
                recitem3.time = time3;
                mtlsdqueue.push_back(recitem3);
                EV_DEBUG << "Third time-location claim (" << claimID << ",( " << position3x << ", " << position3y << ", " << position3z << "), " << time3 << ")" << endl;

            }

            claimID = recMtlsd->getClaimId();
            IMobility *mobility = check_and_cast<IMobility *>(parent->getSubmodule("mobility"));
            Coord Speed = mobility->getCurrentVelocity();
            double Sp = mobility->getMaxSpeed();
            EV << "Max speed is : " << Sp << endl;
            speedX = Speed.getX();
            speedY = Speed.getY();
            speedZ = Speed.getZ();
            EV << "SpeedX = " << speedX << ", SpeedY = " << speedY << ", SpeedZ = " << speedZ << endl;
            for (auto i:mtlsd_file)
            {
                if (i.ID == claimID)
                {
                    EV_DEBUG <<  "Node for which we detect is " << claimID << endl;
                    EV_DEBUG << "size of received " << mtlsdqueue.size() << endl;
                    while (!mtlsdqueue.empty())
                    {
                        for(auto it=i.q.begin(); it!=i.q.end();++it)
                        {
                            EV_DEBUG << "size of what i have " << i.q.size() << endl;
                            double px, py, pz;
                            px = mtlsdqueue.front().positionx;
                            py = mtlsdqueue.front().positiony;
                            pz = mtlsdqueue.front().positionz;
                            simtime_t t = mtlsdqueue.front().time;
                            double p_x, p_y, p_z;
                            p_x = it->positionx;
                            p_y = it->positiony;
                            p_z = it->positionz;
                            simtime_t t_ = it->time;
                            double diffx = abs(int(px - p_x));
                            double diffy = abs(int(py - p_y));
                            double diffz = abs(int(pz - p_z));
                            double diffT = abs((t - t_).dbl());
                            EV_DEBUG << "I have (" << p_x << ", " << p_y << ", " << p_z << "), " << t_ << endl;
                            EV_DEBUG << "I received (" << px << ", " << py << ", " << pz << "), " << t << endl;
                            if ((diffx > diffT * Sp) || (diffy > diffT * Sp) || (diffz > diffT * Sp))
                            {
                                EV_DEBUG << "Clone attack detection : Node " << claimID << " has been cloned " << endl;
                                endSimulation();
                            }
                        }
                        mtlsdqueue.pop_front();
                    }
                }
            }
            mtlsdqueue.clear();
            delete msg;
        }

    }
    else
        throw cRuntimeError("Message arrived on unknown gate %s", msg->getArrivalGate()->getName());
}


