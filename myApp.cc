/*
 * myApp.cc
 *
 *  Created on: May 27, 2019
 *      Author: bouchra
 */

#include "myApp.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/applications/udpapp/UdpBasicApp.h"
#include "inet/common/TagBase_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include <iterator>

using namespace std;
using namespace inet;
Define_Module(myApp);

myApp::~myApp()
{
    cancelAndDelete(event);
    cancelAndDelete(event2);
    cancelAndDelete(event3);
}

void myApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        sequencenumber = 0;
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        event = new cMessage("event");
        broadcastDelay = &par("broadcastDelay");
        helloInterval = par("helloInterval");
        localPort = par("localPort");
        destPort = par("destPort");
        packetName = par("packetName");
        startTime = par("startTime");
        stopTime = par("stopTime");
        parent = getParentModule();
        grandparent = parent->getParentModule();
        num_nodes = grandparent->par("numHosts");
        nodeID = parent->getId();
        moduleType = cModuleType::get("Drone");
        if (parent->getName() != "clone")
        {
            parent->par("myID") = nodeID;
        }
        myID = parent->par("myID");
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS)
    {
        registerService(Protocol::manet, nullptr, gate("socketIn"));
        registerProtocol(Protocol::manet, gate("socketOut"), nullptr);
    }
}

void myApp::handleSelfMessage(cMessage *msg)
{

}

void myApp::handleMessageWhenUp(cMessage *msg)
{

}

void myApp::start()
{
    int  num_80211 = 0;
    InterfaceEntry *ie;
    InterfaceEntry *i_face;
    const char *name;
    for (int i = 0; i < ift->getNumInterfaces(); i++)
    {
        ie = ift->getInterface(i);
        name = ie->getInterfaceName();
        if (strstr(name, "wlan") != nullptr)
        {
            i_face = ie;
            num_80211++;
            interfaceId = i;
        }
    }

    if (num_80211 == 1)
    {
        interface80211ptr = i_face;
        interfaceId = interface80211ptr->getInterfaceId();
    }
    else
        throw cRuntimeError("Node has found %i 80211 interfaces", num_80211);

    scheduleAt(simTime() + uniform(0.0, par("maxVariance").doubleValue()), event);
}

void myApp::stop()
{
    cancelEvent(event);
    cancelEvent(event2);
    cancelEvent(event3);
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

Packet *myApp::generateHelloPacket()
{
    auto hello = makeShared<HelloMsg>();
    Ipv4Address source = (interface80211ptr->getProtocolData<Ipv4InterfaceData>()->getIPAddress());
    hello->setChunkLength(b(128));
    hello->setSrcAddress(source);
    sequencenumber += 2;
    hello->setSequencenumber(sequencenumber);
    hello->setNextAddress(source);
    hello->setHopdistance(1);
    hello->setId(int(parent->par("myID")));
    auto packet = new Packet("Hello", hello);
    packet->addTagIfAbsent<L3AddressReq>()->setDestAddress(Ipv4Address(255, 255, 255, 255));
    packet->addTagIfAbsent<L3AddressReq>()->setSrcAddress(source);
    packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(interface80211ptr->getInterfaceId());
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::manet);
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    return packet;
}
void myApp::generateClone()
{
    module = moduleType->create("clone", grandparent);
    module->finalizeParameters();
    module->buildInside();
    double Time = intuniform(1, 50);
    Time = Time/100;
    Time = SIMTIME_DBL(simTime())+Time;
    module->scheduleStart(simTime() + Time);
    cPreModuleInitNotification pre;
    pre.module = module;
    emit(POST_MODEL_CHANGE, &pre);
    module->callInitialize();
    cPostModuleInitNotification post;
    post.module = module;
    emit(POST_MODEL_CHANGE, &post);
    module->par("myID") = parent->getId();
    module->setName("clone");
    parent->setName("cloned");
}


