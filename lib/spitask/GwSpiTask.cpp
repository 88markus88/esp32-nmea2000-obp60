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
#include "GwSpiTask.h"
#include "GwSpiSensor.h"
#include "GWDMS22B.h"
#include "GwTimer.h"

static SPIBus bus1(GWSPIHOST1);
static SPIBus bus2(GWSPIHOST2);

static SpiSensorList sensors;

#ifdef GWSPI1_CLK
static const int spi1clk=GWSPI1_CLK;
#else
static const int spi1clk=-1;
#endif
#ifdef GWSPI1_MISO
static const int spi1miso=GWSPI1_MISO;
#else
static const int spi1miso=-1;
#endif
#ifdef GWSPI1_MOSI
static const int spi1mosi=GWSPI1_MOSI;
#else
static const int spi1mosi=-1;
#endif

#ifdef GWSPI2_CLK
static const int spi2clk=GWSPI2_CLK;
#else
static const int spi2clk=-1;
#endif
#ifdef GWSPI2_MISO
static const int spi2miso=GWSPI2_MISO;
#else
static const int spi2miso=-1;
#endif
#ifdef GWSPI2_MOSI
static const int spi2mosi=GWSPI2_MOSI;
#else
static const int spi2mosi=-1;
#endif

#define _GWSPI
void runSpiTask(GwApi *api){
    GwLog *logger=api->getLogger();
    std::map<int,SPIBus *> buses;
    for (auto && sensor:sensors){
        int busId=sensor->busId;
        auto bus=buses.find(busId);
        if (bus == buses.end()){
            switch (busId)
            {
            case 1:
                if (spi1clk < 0){
                    LOG_DEBUG(GwLog::ERROR,"SPI bus 1 not configured, cannot create %s",sensor->prefix.c_str());
                }
                else{
                    if (bus1.init(logger,spi1miso,spi1mosi,spi1clk)){
                        buses[busId]=&bus1;
                    }
                }
                break;
            case 2:
                if (spi2clk < 0){
                    LOG_DEBUG(GwLog::ERROR,"SPI bus 2 not configured, cannot create %s",sensor->prefix.c_str());
                }
                else{
                    if (bus2.init(logger,spi2miso,spi2mosi,spi2clk)){
                        buses[busId]=&bus2;
                    }
                }
                break;
            default:
                LOG_DEBUG(GwLog::ERROR,"invalid busid %d for %s",busId,sensor->prefix.c_str());
            }
        }
    }
    GwConfigHandler *config=api->getConfig();
    bool runLoop=false;
    GwIntervalRunner timers;
    int counterId=api->addCounter("spisensors");
    for (auto && sensor:sensors){
        if (!sensor->isActive()) continue;
        auto bus=buses.find(sensor->busId);
        if (bus == buses.end()){
            continue;
        }
        bool rt=sensor->initDevice(api,bus->second);
        if (rt){
            runLoop=true;
            timers.addAction(sensor->intv,[bus,api,sensor,counterId](){
                sensor->measure(api,bus->second,counterId);
            });
        }
    }
    if (! runLoop){
        LOG_DEBUG(GwLog::LOG,"nothing to do for SPI task, finish");
        vTaskDelete(NULL);
        return;
    }
    while(true){
        delay(100);
        timers.loop();
    }
    vTaskDelete(NULL);
}


void initSpiTask(GwApi *api){
    GwLog *logger=api->getLogger();
    #ifndef _GWSPI
        return;
    #endif
    registerDMS22B(api,sensors);
    bool addTask=false;
    for (auto && sensor:sensors){
        if (sensor->preinit(api)) addTask=true;
    }
    if (addTask){
        api->addUserTask(runSpiTask,"spiTask",3000);   
    }
    else{
        LOG_DEBUG(GwLog::LOG,"no SPI sensors defined");
    }
}