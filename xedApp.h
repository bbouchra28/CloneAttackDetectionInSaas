/*
 * xedApp.h
 *
 *  Created on: Jun 12, 2019
 *      Author: bouchra
 */

#ifndef XEDAPP_H_
#define XEDAPP_H_

#include "myApp.h"
#include "XedMsg_m.h"

class XED;
class xedApp;

/********** XED **********/
class XED
{
  public:
    L3Address originatorAddr, destinationAddr;
    unsigned int random;
    XED(const L3Address& originatorAddr, const L3Address& destinationAddr, unsigned int random)
    : originatorAddr(originatorAddr), destinationAddr(destinationAddr), random(random) {};
    bool operator==(const XED& other) const
    {
        return this->originatorAddr == other.originatorAddr && this->destinationAddr == other.destinationAddr
                && this->random == other.random;
    }
};

class xedApp : public myApp
{
    protected:
        list<XED> lr,ls;

    protected:
        virtual void initialize(int stage) override;
        virtual void handleMessageWhenUp(cMessage *msg) override;

        void handleSelfMessage(cMessage *msg);
        double generateRandom();

    public:
       xedApp() {}
       ~xedApp() {}
};

#endif /* XEDAPP_H_ */
