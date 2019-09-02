/*
 * pathApp.h
 *
 *  Created on: May 27, 2019
 *      Author: bouchra
 */

#ifndef PATHAPP_H_
#define PATHAPP_H_

#include <functional>
#include "myApp.h"
#include "PathMsg_m.h"

class pathApp;

typedef struct PathItem{
    int ID;
    size_t path;
    int length;
}PATH_ITEM;


class pathApp : public myApp
{
    protected:
        ostringstream path;
        list<PATH_ITEM> received_path;

    protected:
        virtual void initialize(int stage) override;
        virtual void handleMessageWhenUp(cMessage *msg) override;

        void handleSelfMessage(cMessage *msg);

    public:
       pathApp() {}
       ~pathApp() {}
};

#endif /* PATHAPP_H_ */
