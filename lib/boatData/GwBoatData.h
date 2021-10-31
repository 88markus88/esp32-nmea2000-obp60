#ifndef _GWBOATDATA_H
#define _GWBOATDATA_H

#include "GwLog.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <map>
#define GW_BOAT_VALUE_LEN 32
class GwBoatItemBase{
    public:
        static const unsigned long INVALID_TIME=60000;
        typedef std::map<String,GwBoatItemBase*> GwBoatItemMap;
    protected:
        unsigned long lastSet=0;
        unsigned long invalidTime=INVALID_TIME;
        void uls(){
            lastSet=millis();
        }
    public:
        unsigned long getLastSet() const {return lastSet;}
        bool isValid(unsigned long now=0) const {
            if (lastSet == 0) return false;
            if (invalidTime == 0) return true;
            if (now == 0) now=millis();
            return (lastSet + invalidTime) >= now;
        }
        GwBoatItemBase(unsigned long invalidTime=INVALID_TIME){
            lastSet=0;
            this->invalidTime=invalidTime;
        }
        virtual ~GwBoatItemBase(){}
        void invalidate(){
            lastSet=0;
        }
        virtual void toJsonDoc(DynamicJsonDocument *doc,String name)=0;
};
class GwBoatData;
template<class T> class GwBoatItem : public GwBoatItemBase{
    public:
        typedef T (*Formatter)(T);
    private:
        T data;
        Formatter fmt;
    public:
        GwBoatItem(unsigned long invalidTime=INVALID_TIME,Formatter fmt=NULL):
            GwBoatItemBase(invalidTime){
            this->fmt=fmt;
        }
        GwBoatItem(GwBoatItemMap *map,String name,unsigned long invalidTime=INVALID_TIME,Formatter fmt=NULL):
            GwBoatItemBase(invalidTime){
            this->fmt=fmt;
            (*map)[name]=this;
        }
        virtual ~GwBoatItem(){}
        void update(T nv){
            data=nv;
            uls();
        }
        T getData(bool useFormatter=false){
            if (! useFormatter || fmt == NULL) return data;
            return (*fmt)(data);
        }
        T getDataWithDefault(T defaultv){
            if (! isValid(millis())) return defaultv;
            return data;
        }
        virtual void toJsonDoc(DynamicJsonDocument *doc,String name){
            (*doc)[name]=getData(true);
        }
};

static double formatCourse(double cv)
    {
        double rt = cv * 180.0 / M_PI;
        if (rt > 360)
            rt -= 360;
        if (rt < 0)
            rt += 360;
        return rt;
    }
    static double formatWind(double cv)
    {
        double rt = formatCourse(cv);
        if (rt > 180)
            rt = 180 - rt;
        return rt;
    }
    static double formatKnots(double cv)
    {
        return cv * 3600.0 / 1852.0;
    }

    static uint32_t mtr2nm(uint32_t m)
    {
        return m / 1852;
    }

    static double kelvinToC(double v){
        return v-273.15;
    }


#define GWBOATDATA(type,name,time,fmt)  \
    GwBoatItem<type> *name=new GwBoatItem<type>(&values,F(#name),time,fmt) ;
class GwBoatData{
    private:
        GwLog *logger;
        GwBoatItemBase::GwBoatItemMap values;
    public:

    GWBOATDATA(double,COG,4000,&formatCourse)
    GWBOATDATA(double,TWD,4000,&formatCourse)
    GWBOATDATA(double,AWD,4000,&formatCourse)
    GWBOATDATA(double,SOG,4000,&formatKnots)
    GWBOATDATA(double,STW,4000,&formatKnots)
    GWBOATDATA(double,TWS,4000,&formatKnots)
    GWBOATDATA(double,AWS,4000,&formatKnots)
    GWBOATDATA(double,MaxTws,4000,&formatKnots)
    GWBOATDATA(double,MaxAws,4000,&formatKnots)
    GWBOATDATA(double,AWA,4000,&formatWind)
    GWBOATDATA(double,Heading,4000,&formatCourse)
    GWBOATDATA(double,Variation,4000,&formatCourse)
    GWBOATDATA(double,RudderPosition,4000,&formatCourse)
    GWBOATDATA(double,Latitude,4000,NULL)
    GWBOATDATA(double,Longitude,4000,NULL)
    GWBOATDATA(double,Altitude,4000,NULL)
    GWBOATDATA(double,WaterDepth,4000,NULL)
    GWBOATDATA(double,SecondsSinceMidnight,4000,NULL)
    GWBOATDATA(double,WaterTemperature,4000,&kelvinToC)
    GWBOATDATA(uint32_t,Log,0,&mtr2nm)
    GWBOATDATA(uint32_t,TripLog,0,&mtr2nm)
    GWBOATDATA(uint32_t,DaysSince1970,4000,NULL)
    public:
        GwBoatData(GwLog *logger);
        ~GwBoatData();
        String toJson() const;
};


#endif