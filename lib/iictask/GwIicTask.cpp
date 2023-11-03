#include "GwIicTask.h"
#include "GwHardware.h"
#ifdef _GWIIC
    #include <Wire.h>
#endif
#if defined(GWSHT3X) || defined(GWSHT3X1) || defined(GWSHT3X2) || defined(GWSHT3X2) || defined(GWSHT3X4)
    #define _GWSHT3X
#else
    #undef _GWSHT3X
#endif
#ifdef _GWSHT3X
    #include "SHT3X.h"
#endif
#ifdef GWQMP6988
    #include "QMP6988.h"
#endif
#include "GwTimer.h"
#include "N2kMessages.h"
#include "GwHardware.h"
#include "GwXdrTypeMappings.h"
#ifdef GWBME280
    #include <Adafruit_BME280.h>
#endif

#ifndef GWIIC_SDA
    #define GWIIC_SDA -1
#endif
#ifndef GWIIC_SCL
    #define GWIIC_SCL -1
#endif

#define CFG_GET(cfg,name,prefix) \
    cfg->getValue(name, GwConfigDefinitions::prefix ## name)

#define CSHT3X(name) \
    CFG_GET(config,name,SHT3X1)
#define CQMP6988(name) \
    CFG_GET(config,name,QMP69881)
#define CBME280(name) \
    CFG_GET(config,name,BME2801)    
class SHT3XConfig{
    public:
    const String prefix="SHT3X1";
    String tmNam;
    String huNam;
    int iid;
    bool tmAct;
    bool huAct;
    long intv;
    tN2kHumiditySource huSrc;
    tN2kTempSource tmSrc;
    SHT3XConfig(GwConfigHandler *config){
        CSHT3X(tmNam);
        CSHT3X(huNam);
        CSHT3X(iid);
        CSHT3X(tmAct);
        CSHT3X(huAct);
        CSHT3X(intv);
        intv*=1000;
        CSHT3X(huSrc);
        CSHT3X(tmSrc);
    }
};

class QMP6988Config{
    public:
        const String prefix="QMP69881";
        String prNam="Pressure";
        int iid=99;
        bool prAct=true;
        long intv=2000;
        tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
        float prOff=0;
        QMP6988Config(GwConfigHandler *config){
            CQMP6988(prNam);
            CQMP6988(iid);
            CQMP6988(prAct);
            CQMP6988(intv);
            intv*=1000;
            CQMP6988(prOff);
        }
};

class BME280Config{
    public:
    const String prefix="BME2801";
    bool prAct=true;
    bool tmAct=true;
    bool huAct=true;
    tN2kTempSource tmSrc=tN2kTempSource::N2kts_InsideTemperature;
    tN2kHumiditySource huSrc=tN2kHumiditySource::N2khs_InsideHumidity;
    tN2kPressureSource prSrc=tN2kPressureSource::N2kps_Atmospheric;
    int iid=99;
    long intv=2000;
    String tmNam="Temperature";
    String huNam="Humidity";
    String prNam="Pressure";
    float tmOff=0;
    float prOff=0;
    BME280Config(GwConfigHandler *config){
        CBME280(prAct);
        CBME280(tmAct);
        CBME280(huAct);
        CBME280(tmSrc);
        CBME280(huSrc);
        CBME280(iid);
        CBME280(intv);
        intv*=1000;
        CBME280(tmNam);
        CBME280(huNam);
        CBME280(prNam);
        CBME280(tmOff);
        CBME280(prOff);
    }
};

template <class CFG>
bool addPressureXdr(GwApi *api, CFG &cfg)
{
    if (! cfg.prAct) return false;
    if (cfg.prNam.isEmpty()){
        api->getLogger()->logDebug(GwLog::LOG, "pressure active for %s, no xdr mapping", cfg.prefix.c_str());    
        return true;
    }
    api->getLogger()->logDebug(GwLog::LOG, "adding pressure xdr mapping for %s", cfg.prefix.c_str());
    GwXDRMappingDef xdr;
    xdr.category = GwXDRCategory::XDRPRESSURE;
    xdr.direction = GwXDRMappingDef::M_FROM2K;
    xdr.selector = (int)cfg.prSrc;
    xdr.instanceId = cfg.iid;
    xdr.instanceMode = GwXDRMappingDef::IS_SINGLE;
    xdr.xdrName = cfg.prNam;
    api->addXdrMapping(xdr);
    return true;
}

template <class CFG>
bool addTempXdr(GwApi *api, CFG &cfg)
{
    if (!cfg.tmAct) return false;
    if (cfg.tmNam.isEmpty()){
        api->getLogger()->logDebug(GwLog::LOG, "temperature active for %s, no xdr mapping", cfg.prefix.c_str());    
        return true;
    }
    api->getLogger()->logDebug(GwLog::LOG, "adding temperature xdr mapping for %s", cfg.prefix.c_str());
    GwXDRMappingDef xdr;
    xdr.category = GwXDRCategory::XDRTEMP;
    xdr.direction = GwXDRMappingDef::M_FROM2K;
    xdr.field = GWXDRFIELD_TEMPERATURE_ACTUALTEMPERATURE;
    xdr.selector = (int)cfg.tmSrc;
    xdr.instanceMode = GwXDRMappingDef::IS_SINGLE;
    xdr.instanceId = cfg.iid;
    xdr.xdrName = cfg.tmNam;
    api->addXdrMapping(xdr);
    return true;
}

template <class CFG>
bool addHumidXdr(GwApi *api, CFG &cfg)
{
    if (! cfg.huAct) return false;
    if (cfg.huNam.isEmpty()){
        api->getLogger()->logDebug(GwLog::LOG, "humidity active for %s, no xdr mapping", cfg.prefix.c_str());    
        return true;
    }
    api->getLogger()->logDebug(GwLog::LOG, "adding humidity xdr mapping for %s", cfg.prefix.c_str());
    GwXDRMappingDef xdr;
    xdr.category = GwXDRCategory::XDRHUMIDITY;
    xdr.direction = GwXDRMappingDef::M_FROM2K;
    xdr.field = GWXDRFIELD_HUMIDITY_ACTUALHUMIDITY;
    xdr.selector = (int)cfg.huSrc;
    xdr.instanceMode = GwXDRMappingDef::IS_SINGLE;
    xdr.instanceId = cfg.iid;
    xdr.xdrName = cfg.huNam;
    api->addXdrMapping(xdr);
    return true;
}

template <class CFG>
void sendN2kHumidity(GwApi *api,CFG &cfg,double value, int counterId){
    tN2kMsg msg;
    SetN2kHumidity(msg,1,cfg.iid,cfg.huSrc,value);
    api->sendN2kMessage(msg);
    api->increment(counterId,cfg.prefix+String("hum"));
}

template <class CFG>
void sendN2kPressure(GwApi *api,CFG &cfg,double value, int counterId){
    tN2kMsg msg;
    SetN2kPressure(msg,1,cfg.iid,cfg.prSrc,value);
    api->sendN2kMessage(msg);
    api->increment(counterId,cfg.prefix+String("press"));
}

template <class CFG>
void sendN2kTemperature(GwApi *api,CFG &cfg,double value, int counterId){
    tN2kMsg msg;
    SetN2kTemperature(msg,1,cfg.iid,cfg.tmSrc,value);
    api->sendN2kMessage(msg);
    api->increment(counterId,cfg.prefix+String("temp"));
}
void runIicTask(GwApi *api);

void initIicTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWIIC
        vTaskDelete(NULL);
        return;
    #else
    bool addTask=false;
    #ifdef GWSHT3X
        LOG_DEBUG(GwLog::LOG,"SHT3X configured");
        SHT3XConfig sht3xConfig(api->getConfig());
        api->addCapability(sht3xConfig.prefix,"true");
        addHumidXdr(api,sht3xConfig);
        addTempXdr(api,sht3xConfig);
        if (sht3xConfig.tmAct || sht3xConfig.huAct) addTask=true;
    #endif
    #ifdef GWQMP6988
        LOG_DEBUG(GwLog::LOG,"QMP6988 configured");
        QMP6988Config qmp6988Config(api->getConfig());
        api->addCapability(qmp6988Config.prefix,"true");
        addPressureXdr(api,qmp6988Config);
    #endif
    #ifdef GWBME280
        LOG_DEBUG(GwLog::LOG,"BME280 configured");
        BME280Config bme280Config(api->getConfig());
        api->addCapability(bme280Config.prefix,"true");
        bool bme280Active=false;
        if (addPressureXdr(api,bme280Config)) bme280Active=true;
        if (addTempXdr(api,bme280Config)) bme280Active=true;
        if (addHumidXdr(api,bme280Config)) bme280Active=true;
        if (! bme280Active){
            LOG_DEBUG(GwLog::DEBUG,"BME280 configured but disabled");
        }
        else{
            addTask=true;
        }
    #endif
    if (addTask){
        api->addUserTask(runIicTask,"iicTask",3000);
    }
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
    bool runLoop=false;
    GwIntervalRunner timers;
    int counterId=api->addCounter("iicsensors");
    #ifdef GWSHT3X
        SHT3X *sht3x=nullptr;
        int addr=GWSHT3X;
        if (addr < 0) addr=0x44; //default
        SHT3XConfig sht3xConfig(config);
        if (sht3xConfig.huAct || sht3xConfig.tmAct){
            sht3x=new SHT3X();
            sht3x->init(addr,&Wire);
            LOG_DEBUG(GwLog::LOG,"initialized SHT3X at address %d, intv %ld",(int)addr,sht3xConfig.intv);
            runLoop=true;
            timers.addAction(sht3xConfig.intv,[logger,api,sht3x,sht3xConfig,counterId](){
                int rt=0;
                if ((rt=sht3x->get())==0){
                    double temp=sht3x->cTemp;
                    temp=CToKelvin(temp);
                    double humid=sht3x->humidity;
                    LOG_DEBUG(GwLog::DEBUG,"SHT3X measure temp=%2.1f, humid=%2.0f",(float)temp,(float)humid);
                    if (sht3xConfig.huAct){
                        sendN2kHumidity(api,sht3xConfig,humid,counterId);
                    }
                    if (sht3xConfig.tmAct){
                        sendN2kTemperature(api,sht3xConfig,temp,counterId);
                    }
                }
                else{
                    LOG_DEBUG(GwLog::DEBUG,"unable to query SHT3X: %d",rt);
                }    
            });
        }
    #endif
    #ifdef GWQMP6988
        int qaddr=GWQMP6988;
        if (qaddr < 0) qaddr=0x56;
        QMP6988Config qmp6988Config(api->getConfig());
        QMP6988 *qmp6988=nullptr;
        if (qmp6988Config.prAct){
            runLoop=true;
            qmp6988=new QMP6988();
            qmp6988->init(qaddr,&Wire);
            LOG_DEBUG(GwLog::LOG,"initialized QMP6988 at address %d, intv %ld",qaddr,qmp6988Config.intv);
            timers.addAction(qmp6988Config.intv,[logger,api,qmp6988,qmp6988Config,counterId](){
                float pressure=qmp6988->calcPressure();
                float computed=pressure+qmp6988Config.prOff;
                LOG_DEBUG(GwLog::DEBUG,"qmp6988 measure %2.0fPa, computed %2.0fPa",pressure,computed);
                sendN2kPressure(api,qmp6988Config,computed,counterId);
            });
        }
    #endif
    #ifdef GWBME280
        int baddr=GWBME280;
        if (baddr < 0) baddr=0x76;
        BME280Config bme280Config(api->getConfig());
        if (bme280Config.tmAct || bme280Config.prAct|| bme280Config.huAct){
            Adafruit_BME280 *bme280=new Adafruit_BME280();
            if (bme280->begin(baddr,&Wire)){
                uint32_t sensorId=bme280->sensorID();
                bool hasHumidity=sensorId == 0x60; //BME280, else BMP280
                if (bme280Config.tmOff != 0){
                    bme280->setTemperatureCompensation(bme280Config.tmOff);
                }
                if (hasHumidity || bme280Config.tmAct || bme280Config.prAct)
                {
                    LOG_DEBUG(GwLog::LOG, "initialized BME280 at %d, sensorId 0x%x", baddr, sensorId);
                    timers.addAction(bme280Config.intv, [logger, api, bme280, bme280Config, counterId, hasHumidity](){
                        if (bme280Config.prAct){
                            float pressure=bme280->readPressure();
                            float computed=pressure+bme280Config.prOff;
                            LOG_DEBUG(GwLog::DEBUG,"BME280 measure %2.0fPa, computed %2.0fPa",pressure,computed);
                            sendN2kPressure(api,bme280Config,computed,counterId);
                        }
                        if (bme280Config.tmAct){
                            float temperature=bme280->readTemperature(); //offset is handled internally
                            temperature=CToKelvin(temperature);
                            LOG_DEBUG(GwLog::DEBUG,"BME280 measure temp=%2.1f",temperature);
                            sendN2kTemperature(api,bme280Config,temperature,counterId);
                        }
                        if (bme280Config.huAct && hasHumidity){
                            float humidity=bme280->readHumidity();
                            LOG_DEBUG(GwLog::DEBUG,"BME280 read humid=%02.0f",humidity);
                            sendN2kHumidity(api,bme280Config,humidity,counterId);
                        }
                    });
                    runLoop = true;
                }
                else{
                    LOG_DEBUG(GwLog::ERROR,"BME280 only humidity active, but sensor does not have it");
                }
            }
            else{
                LOG_DEBUG(GwLog::ERROR,"unable to initialize BME280 sensor at address %d",baddr);
            }
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
    #endif
}
