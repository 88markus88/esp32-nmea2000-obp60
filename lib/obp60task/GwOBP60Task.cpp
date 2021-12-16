
//we only compile for some boards
#ifdef BOARD_NODEMCU32S_OBP60
#include "GwOBP60Task.h"
#include "GwApi.h"
#include "OBP60Hardware.h"              // PIN definitions
#include <Ticker.h>                     // Timer Lib for timer interrupts
#include <Wire.h>                       // I2C connections
#include <MCP23017.h>                   // MCP23017 extension Port
#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
#include <GxEPD.h>                      // GxEPD lib for E-Ink displays
#include <GxGDEW042T2/GxGDEW042T2.h>    // 4.2" Waveshare S/W 300 x 400 pixel
#include <GxIO/GxIO_SPI/GxIO_SPI.h>     // GxEPD lip for SPI display communikation
#include <GxIO/GxIO.h>                  // GxEPD lip for SPI
#include "OBP60ExtensionPort.h"         // Functions lib for extension board
#include "OBP60Keypad.h"                // Functions lib for keypad

// True type character sets
#include "Ubuntu_Bold8pt7b.h"
#include "Ubuntu_Bold20pt7b.h"
#include "Ubuntu_Bold32pt7b.h"
#include "DSEG7Classic-BoldItalic16pt7b.h"
#include "DSEG7Classic-BoldItalic42pt7b.h"
#include "DSEG7Classic-BoldItalic60pt7b.h"

// Pictures
//#include GxEPD_BitmapExamples         // Example picture
#include "MFD_OBP60_400x300_sw.h"       // MFD with logo
#include "Logo_OBP_400x300_sw.h"        // OBP Logo

#include "OBP60QRWiFi.h"                // Functions lib for WiFi QR code
#include "Page_0.h"                     // Page 0 Depth
#include "Page_1.h"                     // Page 1 Speed
#include "Page_2.h"                     // Page 2 VBat
#include "Page_3.h"                     // Page 3 Depht / Speed
#include "OBP60Pages.h"                 // Functions lib for show pages

tNMEA0183Msg NMEA0183Msg;
tNMEA0183 NMEA0183;

// Timer Interrupts for hardware functions
Ticker Timer1;  // Under voltage detection
Ticker Timer2;  // Keypad
Ticker Timer3;  // Binking flash LED

// Global vars
bool initComplete = false;      // Initialization complete
int taskRunCounter = 0;         // Task couter for loop section
bool gps_ready = false;         // GPS initialized and ready to use

#define INVALID_COORD -99999
class GetBoatDataRequest: public GwMessage{
    private:
        GwApi *api;
    public:
        double latitude;
        double longitude;
        GetBoatDataRequest(GwApi *api):GwMessage(F("boat data")){
            this->api=api;
        }
        virtual ~GetBoatDataRequest(){}
    protected:
        /**
         * this methos will be executed within the main thread
         * be sure not to make any time consuming or blocking operation
         */    
        virtual void processImpl(){
            //api->getLogger()->logDebug(GwLog::DEBUG,"boatData request from example task");
            /*access the values from boatData (see GwBoatData.h)
              by using getDataWithDefault it will return the given default value
              if there is no valid data available
              so be sure to use a value that never will be a valid one
              alternatively you can check using the isValid() method at each boatData item
              */
            latitude=api->getBoatData()->Latitude->getDataWithDefault(INVALID_COORD);
            longitude=api->getBoatData()->Longitude->getDataWithDefault(INVALID_COORD);
        };
};

// Hardware initialisation before start all services
//##################################################
void OBP60Init(GwApi *api){
    GwLog *logger=api->getLogger();

    // Define timer interrupts
//    Timer1.attach_ms(1, underVoltageDetection);     // Maximum speed with 1ms
    Timer2.attach_ms(40, readKeypad);               // Timer value nust grater than 30ms
    Timer3.attach_ms(500, blinkingFlashLED); 

    // Extension port MCP23017
    // Check I2C devices MCP23017
    Wire.begin(OBP_I2C_SDA, OBP_I2C_SCL);
    Wire.beginTransmission(MCP23017_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        LOG_DEBUG(GwLog::ERROR,"MCP23017 not found, check wiring");
        initComplete = false;
    }
    else{ 
        // Start communication
        mcp.init();
        mcp.portMode(MCP23017Port::A, 0b00110000);  //Port A, 0 = out, 1 = in
        mcp.portMode(MCP23017Port::B, 0b11110000);  //Port B, 0 = out, 1 = in

        // Extension Port A set defaults
        setPortPin(OBP_DIGITAL_OUT1, false);    // PA0
        setPortPin(OBP_DIGITAL_OUT2, false);    // PA1
        setPortPin(OBP_FLASH_LED, false);       // PA2
        setPortPin(OBP_BACKLIGHT_LED, false);   // PA3
        setPortPin(OBP_POWER_50, true);         // PA6
        setPortPin(OBP_POWER_33, true);         // PA7

        // Extension Port B set defaults
        setPortPin(PB0, false);     // PB0
        setPortPin(PB1, false);     // PB1
        setPortPin(PB2, false);     // PB2
        setPortPin(PB3, false);     // PB3

        // Settings for 1Wire
        bool enable1Wire = api->getConfig()->getConfigItem(api->getConfig()->use1Wire,true)->asBoolean();
        if(enable1Wire == true){
            LOG_DEBUG(GwLog::DEBUG,"1Wire Mode is On");
        }
        else{
            LOG_DEBUG(GwLog::DEBUG,"1Wire Mode is Off");
        }
        
        // Settings for backlight
        String backlightMode = api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString();
        LOG_DEBUG(GwLog::DEBUG,"Backlight Mode is: %s", backlightMode);
        if(String(backlightMode) == "On"){
            setPortPin(OBP_BACKLIGHT_LED, true);
        }
        if(String(backlightMode) == "Off"){
            setPortPin(OBP_BACKLIGHT_LED, false);
        }
        if(String(backlightMode) == "Control by Key"){
            setPortPin(OBP_BACKLIGHT_LED, false);
        }

        // Settings flash LED mode
        String ledMode = api->getConfig()->getConfigItem(api->getConfig()->flashLED,true)->asString();
        LOG_DEBUG(GwLog::DEBUG,"Backlight Mode is: %s", ledMode);
        if(String(ledMode) == "Off"){
            blinkingLED = false;
        }
        if(String(ledMode) == "Limits Overrun"){
            blinkingLED = true;
        }

        // Start serial stream and take over GPS data stream form internal GPS
        bool gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asBoolean();
        if(gpsOn == true){
            Serial2.begin(9600, SERIAL_8N1, OBP_GPS_TX, -1); // GPS RX unused in hardware (-1)
            if (!Serial2) {     
                LOG_DEBUG(GwLog::ERROR,"GPS modul NEO-6M not found, check wiring");
                gps_ready = false;
                }
            else{
                LOG_DEBUG(GwLog::DEBUG,"GPS modul NEO-M6 found");
                NMEA0183.SetMessageStream(&Serial2);
                NMEA0183.Open();
                gps_ready = true;
            }
        }

        // Marker for init complete
        // Used in OBP60Task()
        initComplete = true;

        // Buzzer tone for initialization finish
        buzPower = uint(api->getConfig()->getConfigItem(api->getConfig()->buzzerPower,true)->asInt());
        buzzer(TONE4, buzPower, 500);

    }
}

// OBP60 Task
//#######################################
void OBP60Task(void *param){

    GwApi *api=(GwApi*)param;
    GwLog *logger=api->getLogger();

    bool hasPosition = false;

    // Get some configuration data from webside
    bool exampleSwitch=api->getConfig()->getConfigItem(api->getConfig()->obp60Config,true)->asBoolean();
    LOG_DEBUG(GwLog::DEBUG,"example switch ist %s",exampleSwitch?"true":"false");
    bool gpsOn=api->getConfig()->getConfigItem(api->getConfig()->useGPS,true)->asBoolean();
    bool bme280On=api->getConfig()->getConfigItem(api->getConfig()->useBME280,true)->asBoolean();
    bool onewireOn=api->getConfig()->getConfigItem(api->getConfig()->use1Wire,true)->asBoolean();
    String powerMode=api->getConfig()->getConfigItem(api->getConfig()->powerMode,true)->asString();
    String displayMode=api->getConfig()->getConfigItem(api->getConfig()->display,true)->asString();
    String backlightMode=api->getConfig()->getConfigItem(api->getConfig()->backlight,true)->asString();

    // Initializing all necessary boat data
    double lastWaterDepth=0;
    bool lastWaterDepthValid=false;
    GwApi::BoatValue *longitude=new GwApi::BoatValue(F("Longitude"));
    GwApi::BoatValue *latitude=new GwApi::BoatValue(F("Latitude"));
    GwApi::BoatValue *waterdepth=new GwApi::BoatValue(F("WaterDepth"));
    GwApi::BoatValue *valueList[]={longitude, latitude, waterdepth};

    //Init E-Ink display
    display.init();                         // Initialize and clear display
    display.setTextColor(GxEPD_BLACK);      // Set display color
    display.setRotation(0);                 // Set display orientation (horizontal)
    display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
    display.update();                       // Full update (slow)
        
    if(displayMode == "Logo + QR Code" || displayMode == "Logo"){
        display.drawExampleBitmap(gImage_Logo_OBP_400x300_sw, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw start logo
//        display.drawExampleBitmap(gImage_MFD_OBP60_400x300_sw, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw start logo
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);   // Partial update (fast)
        delay(SHOW_TIME);                                // Logo show time
        display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);    // Partial update (fast)
        if(displayMode == "Logo + QR Code"){
            qrWiFi();                                        // Show QR code for WiFi connection
            delay(SHOW_TIME);                                // Logo show time
            display.fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE); // Draw white sreen
        }
    }


    // Task Loop
    //###############################
    while(true){
        // Task cycle time
        delay(10);  // 10ms

        // Backlight On/Off Subtask 100ms
        if((taskRunCounter % 10) == 0){
            // If key controled
            if(backlightMode == "Control by Key"){
                if(keystatus == "6s"){
                    LOG_DEBUG(GwLog::DEBUG,"Toggle Backlight LED");
                    togglePortPin(OBP_BACKLIGHT_LED);
                    keystatus = "0";
                }
            }
            // Change page number
            if(keystatus == "5s"){
                pageNumber ++;
                if(pageNumber > MAX_PAGE_NUMBER - 1){
                    pageNumber = 0;
                }
                first_view = true;
                keystatus = "0";
            }
            if(keystatus == "1s"){
                pageNumber --;
                if(pageNumber < 0){
                    pageNumber = MAX_PAGE_NUMBER - 1;
                }
                first_view = true;
                keystatus = "0";
            }
        }

        // Subtask all 1000ms show pages
        if((taskRunCounter % 100) == 0  || first_view == true){
 //           LOG_DEBUG(GwLog::DEBUG,"Subtask 1");
            LOG_DEBUG(GwLog::DEBUG,"Keystatus: %s", keystatus);
            LOG_DEBUG(GwLog::DEBUG,"Pagenumber: %d", pageNumber);
            if(displayMode == "Logo + QR Code" || displayMode == "Logo" || displayMode == "White Screen"){
                showPage();
            }
        }

        // Subtask all 3000ms
        if((taskRunCounter % 300) == 0){
 //           LOG_DEBUG(GwLog::DEBUG,"Subtask 2");
            //Clear swipe code
            if(keystatus == "left" || keystatus == "right"){
                keystatus = "0";
            }
        }

        // Subtask E-Ink full refresh
        if((taskRunCounter % (FULL_REFRESH_TIME * 100)) == 0){
            LOG_DEBUG(GwLog::DEBUG,"E-Ink full refresh");
            display.update(); 
        }

        // Send NMEA0183 GPS data on several bus systems
        if(gpsOn == true){   // If config enabled
                if(gps_ready = true){
                    tNMEA0183Msg NMEA0183Msg;
                    while(NMEA0183.GetMessage(NMEA0183Msg)){
                        api->sendNMEA0183Message(NMEA0183Msg);
                    }
                }
        }

        //fetch the current values of the items that we have in itemNames
        api->getBoatDataValues(3,valueList);
        //check if the values are valid (i.e. the values we requested have been found in boatData)
        if (waterdepth->valid){
            //both values are there - so we have a valid position
            if (! lastWaterDepthValid){
                //access to the values via iterator->second (iterator->first would be the name)
                LOG_DEBUG(GwLog::LOG,"%s new value %f",waterdepth->getName().c_str(),waterdepth->value);
                lastWaterDepthValid=true;
                lastWaterDepth=waterdepth->value;
            }
        }
        else{
            if (lastWaterDepthValid){
                if (exampleSwitch) LOG_DEBUG(GwLog::LOG,"Water depth lost");
                lastWaterDepthValid=false;
            }
        }

        taskRunCounter++;
    }
    vTaskDelete(NULL);
    
}

#endif
