#ifndef _GWAPI_H
#define _GWAPI_H
#include "GwMessage.h"
#include "N2kMsg.h"
#include "NMEA0183Msg.h"
#include "GWConfig.h"
#include "GwBoatData.h"
//API to be used for additional tasks
class GwApi{
    public:
        virtual GwRequestQueue *getQueue()=0;
        virtual void sendN2kMessage(const tN2kMsg &msg)=0;
        virtual void sendNMEA0183Message(const tNMEA0183Msg &msg, int sourceId)=0;
        virtual int getSourceId()=0;
        virtual GwConfigHandler *getConfig()=0;
        virtual GwLog *getLogger()=0;
        virtual GwBoatData *getBoatData()=0;
};
#ifndef DECLARE_USERTASK
#define DECLARE_USERTASK(task)
#endif
#ifndef DECLARE_CAPABILITY
#define DECLARE_CAPABILITY(name,value)
#endif
#endif
