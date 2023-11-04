#include "GwSHT3X.h"

#ifdef _GWSHT3X
#define PRFX1 "SHT3X11"
#define PRFX2 "SHT3X12"
#define PRFX3 "SHT3X21"
#define PRFX4 "SHT3X22"
class SHT3XConfig : public SensorBase{
    public:
    String tmNam;
    String huNam;
    bool tmAct=false;
    bool huAct=false;
    tN2kHumiditySource huSrc;
    tN2kTempSource tmSrc;
    SHT3X *device=nullptr;
    SHT3XConfig(GwApi *api,const String &prefix):
        SensorBase(api,prefix){}
    virtual bool isActive(){
        return tmAct || huAct;
    }
    virtual bool initDevice(GwApi * api,TwoWire *wire){
        if (! isActive()) return false;
        device=new SHT3X();
        device->init(addr,wire);
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"initialized %s at address %d, intv %ld",prefix.c_str(),(int)addr,intv);
        return true;
    }
    virtual bool preinit(GwApi * api){
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"%s configured",prefix.c_str());
        api->addCapability(prefix,"true");
        addHumidXdr(api,*this);
        addTempXdr(api,*this);
        return isActive();
    }
    virtual void measure(GwApi * api,TwoWire *wire, int counterId)
    {
        if (!device)
            return;
        GwLog *logger=api->getLogger();    
        int rt = 0;
        if ((rt = device->get()) == 0)
        {
            double temp = device->cTemp;
            temp = CToKelvin(temp);
            double humid = device->humidity;
            LOG_DEBUG(GwLog::DEBUG, "%s measure temp=%2.1f, humid=%2.0f",prefix.c_str(), (float)temp, (float)humid);
            if (huAct)
            {
                sendN2kHumidity(api, *this, humid, counterId);
            }
            if (tmAct)
            {
                sendN2kTemperature(api, *this, temp, counterId);
            }
        }
        else
        {
            LOG_DEBUG(GwLog::DEBUG, "unable to query %s: %d",prefix.c_str(), rt);
        }
    }
    /**
     * we do not dynamically compute the config names
     * just to get compile time errors if something does not fit
     * correctly
    */
    virtual void readConfig(GwConfigHandler *cfg){
        if (prefix == PRFX1){
            busId=1;
            addr=0x44;
            #undef CG
            #define CG(name) CFG_GET(name,SHT3X11)
            CG(tmNam);
            CG(huNam);
            CG(iid);
            CG(tmAct);
            CG(huAct);
            CG(intv);
            CG(huSrc);
            CG(tmSrc);
            ok=true; 
        }
        if (prefix == PRFX2){
            busId=1;
            addr=0x45;
            #undef CG
            #define CG(name) CFG_GET(name,SHT3X12)
            CG(tmNam);
            CG(huNam);
            CG(iid);
            CG(tmAct);
            CG(huAct);
            CG(intv);
            CG(huSrc);
            CG(tmSrc); 
            ok=true;
        }
        if (prefix == PRFX3){
            busId=2;
            addr=0x44;
            #undef CG
            #define CG(name) CFG_GET(name,SHT3X21)
            CG(tmNam);
            CG(huNam);
            CG(iid);
            CG(tmAct);
            CG(huAct);
            CG(intv);
            CG(huSrc);
            CG(tmSrc); 
            ok=true;
        }
        if (prefix == PRFX4){
            busId=2;
            addr=0x45;
            #undef CG
            #define CG(name) CFG_GET(name,SHT3X22)
            CG(tmNam);
            CG(huNam);
            CG(iid);
            CG(tmAct);
            CG(huAct);
            CG(intv);
            CG(huSrc);
            CG(tmSrc); 
            ok=true;
        }
        intv*=1000;
    }
};
void registerSHT3X(GwApi *api,SensorList &sensors){
    GwLog *logger=api->getLogger();
    #if defined(GWSHT3X) || defined (GWSHT3X11)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,PRFX1);
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWSHT3X12)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,PRFX2);
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWSHT3X21)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,PRFX3);
        sensors.add(api,scfg);
    }
    #endif
    #if defined(GWSHT3X22)
    {
        SHT3XConfig *scfg=new SHT3XConfig(api,PRFX4);
        sensors.add(api,scfg);
    }
    #endif
}
#else
void registerSHT3X(GwApi *api,SensorList &sensors){

}

#endif



