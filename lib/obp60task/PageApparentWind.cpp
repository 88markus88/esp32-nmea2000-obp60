#include "Pagedata.h"

class PageApparentWind : public Page
{
    int dummy=0; //an example on how you would maintain some status
                 //for a page
public:
    PageApparentWind(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"created PageApparentWind");
        dummy=1;
    }
    virtual int handleKey(int key){
        if(key == 3){
            dummy++;
            return 0; // Commit the key
        }
        return key;
    }

    virtual void display(CommonData &commonData, PageData &pageData)
    {
        GwLog *logger = commonData.logger;

        GwConfigHandler *config = commonData.config;
        String test = config->getString(config->lengthFormat);
        

        dummy++;
        for (int i = 0; i < 2; i++)
        {
            GwApi::BoatValue *value = pageData.values[i];
            if (value == NULL)
                continue;
            LOG_DEBUG(GwLog::LOG, "drawing at PageApparentWind(%d),dummy=%d, p=%s,v=%f",
                      i,
                      dummy,
                      value->getName().c_str(),
                      value->valid ? value->value : -1.0);
        }
    };
};

static Page *createPage(CommonData &common){
    return new PageApparentWind(common);
}
/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect (0 here)
 * and will will provide the names of the fixed values we need
 */
PageDescription registerPageApparentWind(
    "apparentWind",
    createPage,
    0,
    {"AWS","AWD"},
    false
);