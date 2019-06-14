/*
 * mtlsdApp.h
 *
 *  Created on: Jun 12, 2019
 *      Author: bouchra
 */

#ifndef MTLSDAPP_H_
#define MTLSDAPP_H_

#include "myApp.h"
#include "MTLSDMsg_m.h"
#include "MTLSDFile_m.h"

class mtlsdApp;

typedef struct Mtlsd{
    Ipv4Address originatorAddr, destinationAddr;
    string position;
    int originatorId;
    simtime_t time;
}MTLSD;

typedef struct Mtlsd_data{
    list<MTLSD> q;
    int ID;
    list<int> witness_nodes;
}MTLSD_DATA;


class mtlsdApp : public myApp
{
    protected:
        list<MTLSD_DATA> mtlsd_file;
        list <MTLSD> mtlsdqueue;
        double maxSpeed;

    protected:
        virtual void initialize(int stage) override;
        virtual void handleMessageWhenUp(cMessage *msg) override;
        void handleSelfMessage(cMessage *msg);

    public:
       mtlsdApp() {}
       ~mtlsdApp() {}
};

#endif /* MTLSDAPP_H_ */
