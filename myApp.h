/*
 * myApp.h
 *
 *  Created on: May 27, 2019
 *      Author: bouchra
 */

#ifndef MYAPP_H_
#define MYAPP_H_

#include "inet/common/INETDefs.h"

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/visualizer/scene/NetworkNodeCanvasVisualizer.h"
#include "inet/common/scenario/ScenarioManager.h"

#include <omnetpp.h>
#include <vector>
#include <random>
#include <algorithm>
#include <queue>
#include <deque>

#include "HelloMsg_m.h"

using namespace omnetpp;
using namespace inet;
using namespace std;

class myApp;

typedef struct Neighbor{
    Ipv4Address adr;
    int id;
}NEIGHBOR;

class myApp : public ApplicationBase
{
    protected:
        enum SelfMsgKinds { START = 1, SEND, STOP };
        int localPort = -1, destPort = -1;
        bool dontFragment = false;
        const char *packetName = nullptr;

        simtime_t startTime;
        simtime_t stopTime;

        cMessage *selfMsg = nullptr;
        cModule *host = nullptr;
        cMessage *event = nullptr;
        cMessage *event2 = nullptr;
        cMessage *event3 = nullptr;
        cPar *broadcastDelay = nullptr;
        unsigned int sequencenumber = 0;
        simtime_t helloInterval;
        IInterfaceTable *ift = nullptr;
        InterfaceEntry *interface80211ptr = nullptr;
        int interfaceId = -1;
        list<NEIGHBOR> neighbors;
        Ipv4Address source;
        int num_nodes;
        int nodeID;
        int myID;

        cModule* parent;
        cModule* grandparent;
        cModule* module;
        cModuleType *moduleType;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessageWhenUp(cMessage *msg) override;

        void handleSelfMessage(cMessage *msg);

        virtual void handleStartOperation(LifecycleOperation *operation) override { start(); }
        virtual void handleStopOperation(LifecycleOperation *operation) override { stop(); }
        virtual void handleCrashOperation(LifecycleOperation *operation) override  { stop(); }
        void start();
        void stop();

        Packet *generateHelloPacket();
        void generateClone();

    public:
       myApp() {}
       ~myApp();
};

#endif /* MYAPP_H_ */
