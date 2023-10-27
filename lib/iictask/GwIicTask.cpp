#include "GwIicTask.h"
#include "GwHardware.h"
#include <Wire.h>
#include <SHT3X.h>
#include "GwTimer.h"
#include "N2kMessages.h"
#include "GwHardware.h"
#include "GwXdrTypeMappings.h"
//#define GWSHT3X -1

#ifndef GWIIC_SDA
    #define GWIIC_SDA -1
#endif
#ifndef GWIIC_SCL
    #define GWIIC_SCL -1
#endif
class SHT3XConfig{
    public:
    String tempTransducer;
    String humidTransducer;
    int iid;
    bool tempActive;
    bool humidActive;
    long interval;
    tN2kHumiditySource humiditySource;
    tN2kTempSource tempSource;
    SHT3XConfig(GwConfigHandler *config){
        tempTransducer=config->getString(GwConfigDefinitions::SHT3XTempName);
        humidTransducer=config->getString(GwConfigDefinitions::SHT3XHumidName);
        iid=config->getInt(GwConfigDefinitions::SHT3Xiid,99);
        tempActive=config->getBool(GwConfigDefinitions::iicSHT3XTemp);
        humidActive=config->getBool(GwConfigDefinitions::iicSHT3XHumid);
        interval=config->getInt(GwConfigDefinitions::SHT3Xinterval);
        interval*=1000;
        humiditySource=N2khs_InsideHumidity;
        tempSource=N2kts_InsideTemperature;
    }
};

void initIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC
        return;
    #endif
    #ifdef GWSHT3X
        api->addCapability("SHT3X","true");
        LOG_DEBUG(GwLog::LOG,"GWSHT3X configured, adding capability and xdr mappings");
        SHT3XConfig sht3xConfig(api->getConfig());
        if (sht3xConfig.humidActive && ! sht3xConfig.humidTransducer.isEmpty()){
            //add XDR mapping for humidity
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRHUMIDITY;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_HUMIDITY_ACTUALHUMIDITY;
            xdr.selector=(int)sht3xConfig.humiditySource;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=sht3xConfig.iid;
            xdr.xdrName=sht3xConfig.humidTransducer;
            api->addXdrMapping(xdr);
        }
        if (sht3xConfig.tempActive && ! sht3xConfig.tempTransducer.isEmpty()){
            //add XDR mapping for humidity
            GwXDRMappingDef xdr;
            xdr.category=GwXDRCategory::XDRTEMP;
            xdr.direction=GwXDRMappingDef::M_FROM2K;
            xdr.field=GWXDRFIELD_TEMPERATURE_ACTUALTEMPERATURE;
            xdr.selector=(int)sht3xConfig.tempSource;
            xdr.instanceMode=GwXDRMappingDef::IS_SINGLE;
            xdr.instanceId=sht3xConfig.iid;
            xdr.xdrName=sht3xConfig.tempTransducer;
            api->addXdrMapping(xdr);
        }
    #endif
}
void runIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC 
        LOG_DEBUG(GwLog::LOG,"no iic defined, iic task stopped");
        vTaskDelete(NULL);
        return;
    #endif
    LOG_DEBUG(GwLog::LOG,"iic task started");
    bool rt=Wire.begin(GWIIC_SDA,GWIIC_SCL);
    if (! rt){
        LOG_DEBUG(GwLog::ERROR,"unable to initialize IIC");
        vTaskDelete(NULL);
        return;
    }
    GwConfigHandler *config=api->getConfig();
    SHT3X *sht3x=nullptr;
    bool runLoop=false;
    GwIntervalRunner timers;
    #ifdef GWSHT3X
        int8_t addr=GWSHT3X;
        if (addr < 0) addr=0x44; //default
        SHT3XConfig sht3xConfig(config);
        if (sht3xConfig.humidActive || sht3xConfig.tempActive){
            sht3x=new SHT3X();
            sht3x->init(addr,&Wire);
            LOG_DEBUG(GwLog::LOG,"initialized SHT3X at address %d, interval %ld",(int)addr,sht3xConfig.interval);
            runLoop=true;
            timers.addAction(sht3xConfig.interval,[logger,api,sht3x,sht3xConfig](){
                int rt=0;
                if ((rt=sht3x->get())==0){
                    double temp=sht3x->cTemp;
                    temp=CToKelvin(temp);
                    double humid=sht3x->humidity;
                    LOG_DEBUG(GwLog::DEBUG,"SHT3X measure temp=%2.1f, humid=%2.0f",(float)temp,(float)humid);
                    tN2kMsg msg;
                    if (sht3xConfig.humidActive){
                        SetN2kHumidity(msg,1,sht3xConfig.iid,sht3xConfig.humiditySource,humid);
                        api->sendN2kMessage(msg);
                    }
                    if (sht3xConfig.tempActive){
                        SetN2kTemperature(msg,1,sht3xConfig.iid,sht3xConfig.tempSource,temp);
                        api->sendN2kMessage(msg);
                    }
                }
                else{
                    LOG_DEBUG(GwLog::DEBUG,"unable to query SHT3X: %d",rt);
                }    
            });
        }
    #endif
    if (! runLoop){
        LOG_DEBUG(GwLog::LOG,"nothing to do for IIC task, finish");
        vTaskDelete(NULL);
        return;
    }
    while(true){
        delay(100);
        timers.loop();
    }
    vTaskDelete(NULL);
}
