/*
  (C) Andreas Vogel andreas@wellenvogel.de
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "GWDMS22B.h"
#include "GwApi.h"
#include "N2kMessages.h"


#define CHECK_BUS(BUS) \
    checkDef("missing config for " #BUS,GW ## BUS ## _CLK ,GW ## BUS ## _MISO);

#define ADD22B(PRFX,BUS) \
        {\
        CHECK_BUS(BUS); \
        GWDMS22B *dms=new GWDMS22B(api,#PRFX,GW ## BUS ## _HOST);\
        sensors.add(api,dms); \
        }

#ifdef GWDMS22B11
    #define ADD22B11 ADD22B(DMS22B11,SPI0)
    #ifndef GWDMS22B11_CS
        #define GWDMS22B11_CS -1
    #endif
#else
    #define GWDMS22B11_CS -1
    #define ADD22B11
#endif

#ifdef GWDMS22B12
    #define ADD22B12 ADD22B(DMS22B12,SPI0)
    #ifndef GWDMS22B12_CS
        #error "you need to define GWDMS22B12_CS"
    #endif
    #if GWDMS22B11_CS == -1
        #error "multiple devices on one SPI bus need chip select defines - GWDMS22B11_CS is unset"
    #endif
#else
    #define GWDMS22B12_CS -1
    #define ADD22B12
#endif

#ifdef GWDMS22B13
    #define ADD22B13 ADD22B(DMS22B13,SPI0)
    #ifndef GWDMS22B13_CS
        #error "you need to define GWDMS22B13_CS"
    #endif
    #if GWDMS22B11_CS == -1
        #error "multiple devices on one SPI bus need chip select defines - GWDMS22B11_CS is unset"
    #endif
#else
    #define GWDMS22B13_CS -1
    #define ADD22B13
#endif

#ifdef GWDMS22B21
    #define ADD22B21 ADD22B(DMS22B21,SPI1)
    #ifndef GWDMS22B21_CS
        #define GWDMS22B21_CS -1
    #endif
#else
    #define GWDMS22B21_CS -1
    #define ADD22B21
#endif

#ifdef GWDMS22B22
    #define ADD22B22 ADD22B(DMS22B22,SPI1)
    #ifndef GWDMS22B22_CS
        #error "you need to define GWDMS22B22_CS"
    #endif
    #if GWDMS22B21_CS == -1
        #error "multiple devices on one SPI bus need chip select defines - GWDMS22B21_CS is unset"
    #endif
#else
    #define GWDMS22B22_CS -1
    #define ADD22B22
#endif

#ifdef GWDMS22B23
    #define ADD22B23 ADD22B(DMS22B23,SPI1)
    #ifndef GWDMS22B23_CS
        #error "you need to define GWDMS22B23_CS"
    #endif
    #if GWDMS22B21_CS == -1
        #error "multiple devices on one SPI bus need chip select defines - GWDMS22B11_CS is unset"
    #endif
#else
    #define GWDMS22B23_CS -1
    #define ADD22B23
#endif



class GWDMS22B : public SSISensor{
    int zero=2047;
    bool invt=false;
    public:
    using SSISensor::SSISensor;
    virtual bool preinit(GwApi * api){
        GwLog *logger=api->getLogger();
        LOG_DEBUG(GwLog::LOG,"DMS22B configured, prefix=%s, intv=%f",prefix.c_str(),fintv);
        api->addCapability(prefix,"true");
        return true;
    }
    virtual void measure(GwApi * api,BusType *bus, int counterId){
        GwLog *logger=api->getLogger();
        uint32_t value=0;
        esp_err_t res=readData(value);
        if (res != ESP_OK){
            LOG_DEBUG(GwLog::ERROR,"unable to measure %s: %d",prefix.c_str(),(int)res);
        }
        double resolved=(((int)value-zero)*360.0/mask);
        if (invt) resolved=-resolved;
        LOG_DEBUG(GwLog::DEBUG,"measure %s : %d, resolved: %f",prefix.c_str(),value,(float)resolved);
        tN2kMsg msg;
        SetN2kRudder(msg,DegToRad(resolved),iid);
        api->sendN2kMessage(msg);
    }
    #define DMS22B(PRFX,...) \
        if (prefix == #PRFX) {\
            CFG_GET(act,PRFX); \
            CFG_GET(iid,PRFX); \
            CFG_GET(fintv,PRFX); \
            CFG_GET(zero,PRFX); \
            CFG_GET(invt,PRFX); \
            bits=12; \
            clock=500000; \
            cs=GW ## PRFX ## _CS; \
            __VA_ARGS__ \
        }
    
    
    virtual void readConfig(GwConfigHandler *cfg){
        DMS22B(DMS22B11);
        DMS22B(DMS22B12);
        DMS22B(DMS22B13);
        DMS22B(DMS22B21);
        DMS22B(DMS22B22);
        DMS22B(DMS22B23);
        intv=1000*fintv;
    }
};

void registerDMS22B(GwApi *api,SpiSensorList &sensors){
    ADD22B11
    ADD22B12
    ADD22B13
    ADD22B21
    ADD22B22
    ADD22B23

}
