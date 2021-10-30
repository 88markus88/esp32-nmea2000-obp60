#include <ESPmDNS.h>
#include "GwWebServer.h"
#include <map>

class EmbeddedFile;
static std::map<String,EmbeddedFile*> embeddedFiles;
class EmbeddedFile {
  public:
    const uint8_t *start;
    int len;
    EmbeddedFile(String name,const uint8_t *start,int len){
      this->start=start;
      this->len=len;
      embeddedFiles[name]=this;
    }
} ;
#define EMBED_GZ_FILE(fileName, fileExt) \
  extern const uint8_t  fileName##_##fileExt##_File[] asm("_binary_generated_" #fileName "_" #fileExt "_gz_start"); \
  extern const uint8_t  fileName##_##fileExt##_FileLen[] asm("_binary_generated_" #fileName "_" #fileExt "_gz_size"); \
  const EmbeddedFile fileName##_##fileExt##_Config(#fileName "." #fileExt,(const uint8_t*)fileName##_##fileExt##_File,(int)fileName##_##fileExt##_FileLen);

EMBED_GZ_FILE(index,html)
EMBED_GZ_FILE(config,json)

void sendEmbeddedFile(String name,String contentType,AsyncWebServerRequest *request){
    std::map<String,EmbeddedFile*>::iterator it=embeddedFiles.find(name);
    if (it != embeddedFiles.end()){
      EmbeddedFile* found=it->second;
      AsyncWebServerResponse *response=request->beginResponse_P(200,contentType,found->start,found->len);
      response->addHeader(F("Content-Encoding"), F("gzip"));
      request->send(response);
    }
    else{
      request->send(404, "text/plain", "Not found");
    }  
  }


GwWebServer::GwWebServer(GwLog* logger,int port){
    server=new AsyncWebServer(port);
    queue=xQueueCreate(10,sizeof(Message *));
    this->logger=logger;
}
void GwWebServer::begin(){
    server->onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      sendEmbeddedFile("index.html","text/html",request);
    });
    server->on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request){
      sendEmbeddedFile("config.json","application/json",request);
    });
    server->begin();
    LOG_DEBUG(GwLog::LOG,"HTTP server started");
    MDNS.addService("_http","_tcp",80);

}
GwWebServer::~GwWebServer(){
    server->end();
    delete server;
    vQueueDelete(queue);
}
void GwWebServer::handleAsyncWebRequest(AsyncWebServerRequest *request, RequestMessage *msg)
{
  msg->ref(); //for the queue
  if (!xQueueSend(queue, &msg, 0))
  {
    Serial.println("unable to enqueue");
    msg->unref(); //queue
    msg->unref(); //our
    request->send(500, "text/plain", "queue full");
    return;
  }
  LOG_DEBUG(GwLog::DEBUG + 1, "wait queue");
  if (msg->wait(500))
  {
    LOG_DEBUG(GwLog::DEBUG + 1, "request ok");
    request->send(200, msg->getContentType(), msg->getResult());
    msg->unref();
    return;
  }
  LOG_DEBUG(GwLog::DEBUG + 1, "switching to async");
  //msg is handed over to async handling
  bool finished = false;
  AsyncWebServerResponse *r = request->beginChunkedResponse(
      msg->getContentType(), [this,msg, finished](uint8_t *ptr, size_t len, size_t len2) -> size_t
      {
        LOG_DEBUG(GwLog::DEBUG + 1, "try read");
        if (msg->isHandled() || msg->wait(1))
        {
          int rt = msg->consume(ptr, len);
          LOG_DEBUG(GwLog::DEBUG + 1, "async response available, return %d\n", rt);
          return rt;
        }
        else
          return RESPONSE_TRY_AGAIN;
      },
      NULL);
  request->onDisconnect([this,msg](void)
                        {
                          LOG_DEBUG(GwLog::DEBUG + 1, "onDisconnect");
                          msg->unref();
                        });
  request->send(r);
}
bool GwWebServer::registerMainHandler(const char *url,RequestCreator creator){
    server->on(url,HTTP_GET, [this,creator,url](AsyncWebServerRequest *request){
        RequestMessage *msg=(*creator)(request);
        if (!msg){
            LOG_DEBUG(GwLog::DEBUG,"creator returns NULL for %s",url);
            request->send(404, "text/plain", "Not found");
            return;
        }
        handleAsyncWebRequest(request,msg);
    });
}
//to be called from the main loop
void GwWebServer::fetchMainRequest()
{
    Message *msg = NULL;
    if (xQueueReceive(queue, &msg, 0))
    {
        LOG_DEBUG(GwLog::DEBUG + 1, "main message");
        msg->process();
        msg->unref();
    }
}
