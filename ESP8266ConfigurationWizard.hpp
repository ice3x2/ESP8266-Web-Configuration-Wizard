#pragma once 


#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <WifiServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include "Config.hpp"
#include "Resources.hpp"
#include "LinkedList.hpp"

// ArduinoJson >= 6.19.1 
// PubSubClient >= 2.8.0

#define CONFIG_FILENAME "/config002.json"
#define OPTIONS_FILENAME "/options002.csv"
#define OLD_OPTIONS_FILENAME "/options001.csv"

#define MQTT_RECONNECT_INTERVAL 3000
#define MQTT_SOCKET_TIMEOUT 3000
#define MQTT_KEPP_ALIVE 1000

#define NTP_RECONNECT_INTERVAL 500

#define CONFIG_FILE_SIZE 4096

#define WIFI_TIMEOUT 60000 //ms


#define MODE_PREPARE 0
#define MODE_CONFIGURATION 1
#define MODE_RUN 2


#define STATUS_PRE 100

#define WIFI_CONNECT_TRY 1
#define WIFI_ERROR -1
#define WIFI_CONNECTED 10

#define NTP_CONNECT_TRY 2
#define NTP_ERROR -2
#define NTP_CONNECTED 20

#define MQTT_CONNECT_TRY 3
#define MQTT_ERROR -3
#define MQTT_CONNECTED 30
#define STATUS_OK 0

class ESP8266ConfigurationWizard {

  private :

    WiFiUDP _udp;
    WiFiClient _wifiClient; 
    ESP8266WebServer* _webServer;
    PubSubClient* _mqtt;
    NTPClient* _ntpClient;
    Config _config;
    String _ipAddress = "0.0.0.0";
    uint16_t _wifiCount = 0;
    uint8_t _mode = MODE_PREPARE;
    int _status = STATUS_PRE;
    
    LinkedList<String> _subscribeList;

    IPAddress _timeServerIP;

    typedef const char* (*option_filter)(const char* name,const char* value);
    typedef void (*status_callback)(int);
    
    option_filter _onFilterOption = NULL;
    status_callback _onStatusCallback = NULL;

    long _startWiFiConnectMillis;
    bool _availableConfigFile;


 public:
    ESP8266ConfigurationWizard(WiFiUDP udp,WiFiClient client);
    void setOnFilterOption(option_filter filter);
    void setOnStatusCallback(status_callback callback);
    Config& getConfig();
    Config* getConfigPt();
    void setConfig(Config config);
    bool available();
    bool availableWifi();
    bool availableNTP();
    bool availableMqtt();
    void d);
    void startConfigurationMode();
    bool isConfigurationMode();
    PubSubClient* pubSubClient();
    void loop();
    int getHours();
    int getMinutes();
    int getSeconds();
    int getDay();
    unsigned long getEpochTime();

    bool loadConfig();

  private :

  void setStatus(int status);
  void connectWiFi();
  bool connectMQTT();
  bool connectNTP(const char* ntpServer,long timeOffset, unsigned long interval );
  void releaseWebServer();
  
  void initConfigurationMode();

  String resultStringFromEncryptionType(int thisType);
    
  void sendBadRequest();
  void onHttpRequestScanWifiCount();
  void onHttpRequestScanWifiItem();
  void onHttpRequestScanWifi();
  void onHttpRequestWifiHtml();
  void onHttpRequestTimeHtml();
  void onHttpRequestMqttHtml();
  void onHttpRequestOptionHtml();
  void onHttpRequestFinishHtml();
  void onHttpRequestAppJs();
  void onHttpRequestEnvJs();
  void onHttpRequestAjaxJs();
  void onHttpRequestMainCss();
  void onHttpRequestInfo();
  void onHttpRequestWifiConnect();
  void onHttpRequestMqttConnect();
  void onHttpRequestMqttInfo();
  void onHttpRequestSelectedSSID();
  
  void onHttpRequestNTPInfo();
  void onHttpRequestSetNTP();
  void onHttpRequestOptionCount();
  void onHttpRequestSetOption();
  void onHttpRequestGetOption();
  void onHttpRequestCommit();

  bool saveConfig();
  bool saveConfigOptions();
  bool loadConfigOptions();
  

};






ESP8266ConfigurationWizard::ESP8266ConfigurationWizard(WiFiUDP udp,WiFiClient client) : _udp(udp), _wifiClient(client),  _mqtt(NULL), _webServer(NULL), _ntpClient(NULL)
,_availableConfigFile(false)
 {

  //_availableConfigFile = loadConfig();
  _mqtt = new PubSubClient(_wifiClient); 
}

void ESP8266ConfigurationWizard::setOnFilterOption(option_filter filter) {
  _onFilterOption = filter;
}

void ESP8266ConfigurationWizard::setOnStatusCallback(status_callback callback) {
  _onStatusCallback = callback;
}

Config& ESP8266ConfigurationWizard::getConfig() {
  return _config;
}

Config* ESP8266ConfigurationWizard::getConfigPt() {
  return &_config;
}

void ESP8266ConfigurationWizard::setConfig(Config config) {
  _config = config;
}


bool ESP8266ConfigurationWizard::available() {
  return availableWifi() && availableNTP() && availableMqtt();
}

bool ESP8266ConfigurationWizard::availableNTP() {
  return _ntpClient != NULL && _ntpClient->isTimeSet();
}

bool ESP8266ConfigurationWizard::availableMqtt() {
  return _mqtt != NULL && _mqtt->connected();
}

bool ESP8266ConfigurationWizard::availableWifi() {
  if(WiFi.status() == WL_CONNECTED) {
    return true;  
  }
  return false; 
}


void ESP8266ConfigurationWizard::connect() {
  loadConfig();
  if(!_availableConfigFile) {
    startConfigurationMode();
    return;
  }
  releaseWebServer();
  _mode = MODE_RUN;
  _mqtt->setSocketTimeout(MQTT_SOCKET_TIMEOUT);
  _mqtt->setKeepAlive(MQTT_KEEPALIVE);

  setStatus(WIFI_CONNECT_TRY);
  connectWiFi();

  while ((WiFi.status() != WL_CONNECTED) && millis() - _startWiFiConnectMillis < WIFI_TIMEOUT) {
    delay(500);
  }
  if(WiFi.status() != WL_CONNECTED) {
      setStatus(WIFI_ERROR);
      return;
  } 
  setStatus(WIFI_CONNECTED);
  
  if(!availableNTP()) {
    setStatus(NTP_CONNECT_TRY);
    delay(100);
    connectNTP(_config.getNTPServer(), _config.getTimeOffset(), (long)_config.getNTPUpdateInterval() * 60000L);
    if(!availableNTP()) {
      setStatus(NTP_ERROR);
      return;
    }
  }
  setStatus(NTP_CONNECTED);
  if(!availableMqtt()) {
    setStatus(MQTT_CONNECT_TRY);
    connectMQTT();
    if(!availableMqtt()) {
      setStatus(MQTT_ERROR);
      return;
    }
  }
  setStatus(MQTT_CONNECTED);
  

  setStatus(STATUS_OK);
  
}

void ESP8266ConfigurationWizard::startConfigurationMode() {
  _mode = MODE_CONFIGURATION;
  initConfigurationMode();
}

bool ESP8266ConfigurationWizard::isConfigurationMode() {
  return _mode == MODE_CONFIGURATION;
}

PubSubClient* ESP8266ConfigurationWizard::pubSubClient() {
    return _mqtt;
}

void ESP8266ConfigurationWizard::loop() {

  if(_mode == MODE_CONFIGURATION) {
    _webServer->handleClient();
    return;
  }
  if(_status == WIFI_CONNECT_TRY && !availableWifi()) {
    if(millis() - _startWiFiConnectMillis < WIFI_TIMEOUT) {
      return;
    } else {
      setStatus(WIFI_ERROR);
    }    
  } else if(!availableWifi()) {
      setStatus(WIFI_CONNECT_TRY);
      connectWiFi();
      return;
  }
  if(_status == WIFI_CONNECT_TRY) {
    setStatus(WIFI_CONNECTED);
  }

  if(!availableNTP()) {
      setStatus(NTP_CONNECT_TRY);
      connectNTP(_config.getNTPServer(), _config.getTimeOffset(), (long)_config.getNTPUpdateInterval() * 60000L);
      if(!availableNTP()) {
        setStatus(NTP_ERROR);
        return;
      }
  }

  if(_status == NTP_CONNECT_TRY) {
    setStatus(NTP_CONNECTED);
  }
  

  _ntpClient->update();
  
  if(!availableMqtt()) {
      setStatus(MQTT_CONNECT_TRY);
      connectMQTT();
      if(!availableMqtt()) {
        setStatus(MQTT_ERROR);
        return;
      }
  }

  if(_status == MQTT_CONNECT_TRY) {
    setStatus(MQTT_CONNECTED);
  }

  _mqtt->loop();
  
}


int ESP8266ConfigurationWizard::getHours() {
  if(_ntpClient == NULL) return -1;
  return _ntpClient->getHours();
}

int ESP8266ConfigurationWizard::getMinutes() {
  if(_ntpClient == NULL) return -1;
  return _ntpClient->getMinutes();
}

int ESP8266ConfigurationWizard::getSeconds() {
  if(_ntpClient == NULL) return -1;
  return _ntpClient->getSeconds();
}

int ESP8266ConfigurationWizard::getDay() {
 if(_ntpClient == NULL) return -1;
  return _ntpClient->getDay();
}

unsigned long ESP8266ConfigurationWizard::getEpochTime() {
  if(_ntpClient == NULL) return -1;
  return _ntpClient->getEpochTime();
}


void ESP8266ConfigurationWizard::setStatus(int status) {
  if(_status == status) {
    return;
  }
  _status = status;
  if(_onStatusCallback != NULL) {
    _onStatusCallback(status);
  }
}

void ESP8266ConfigurationWizard::connectWiFi() {
    _startWiFiConnectMillis = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(_config.getWiFiSSID(), _config.getWiFiPassword());
}

bool ESP8266ConfigurationWizard::connectMQTT() {      
    const char* server = _config.getMQTTAddress();
    int port = _config.getMQTTPort();
    const char* id = _config.getMQTTClientID();
    const char* user = _config.getMQTTUser();
    const char* password = _config.getMQTTPassword();
    
    _mqtt->setServer(server,port);     
    if(!_mqtt->connected()) {
        
        //Serial.print("connect mqtt: ");
        //Serial.println(id);
        //Serial.print("mqtt addr: ");
        //Serial.println(_config.getMQTTAddress());
        //Serial.print("mqtt port: ");
        //Serial.println(_config.getMQTTPort());
        //Serial.print("mqtt user: ");
        //Serial.println(_config.getMQTTUser());
        //Serial.print("mqtt password: ");
        //Serial.println(_config.getMQTTPassword());           
        //Serial.println(strlen(_config.getMQTTUser()));
        
        if(strlen(_config.getMQTTUser()) > 0 && (_mqtt->connect(id,user, password) || _mqtt->connected())) {
              //Serial.println("mqtt connected");
              return true;
        } else if (_mqtt->connect(id) || _mqtt->connected()) {             
            //Serial.println("mqtt connected");
            return true;
        }
        else {
          //Serial.println(_mqtt->state());
          //Serial.println("failed connect mqtt");
          return false;
        }
    }
    return true;
  }

  bool ESP8266ConfigurationWizard::connectNTP(const char* ntpServer,long timeOffset, unsigned long interval ) {
    if(_ntpClient != NULL) {
        _ntpClient->end();
        delete _ntpClient;
        _ntpClient = NULL;
    }
    //long interval = (long)_config.ntpUpdateInterval() * 60000;
    int resultNameServer = WiFi.hostByName(_config.getNTPServer(), _timeServerIP);
    if(resultNameServer == 0) return false;

    _ntpClient = new NTPClient(_udp, ntpServer, timeOffset, interval);
    
    _ntpClient->begin();
    //Serial.println(ntpServer);
    //Serial.println(timeOffset);
    //Serial.println(interval);
    
    long startConnectTime = millis();
    _ntpClient->update();
    if(!_ntpClient->isTimeSet()) {
      delay(NTP_RECONNECT_INTERVAL);
      _ntpClient->setRandomPort();
      _ntpClient->update();
    }
    
    //Serial.println("");
    //Serial.println(_ntpClient->getEpochTime());
    return _ntpClient->isTimeSet();
  
  }


  void ESP8266ConfigurationWizard::releaseWebServer() {
    if(_webServer != NULL) {
        _webServer->close();
        _webServer->stop();
        delete _webServer;
        _webServer = NULL;
    }
  }

  void ESP8266ConfigurationWizard::initConfigurationMode()  {
    releaseWebServer();
    _webServer = new ESP8266WebServer(80);
    



    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(_config.getAPName(), "");
    
    IPAddress myIP = WiFi.softAPIP();
    //Serial.print("AP IP address: ");
    //Serial.println(myIP);

    
    _webServer->on("/", HTTP_GET, [&]{ onHttpRequestWifiHtml(); });
    _webServer->on("/wifi", HTTP_GET, [&]{ onHttpRequestWifiHtml(); });
    _webServer->on("/time", HTTP_GET, [&]{ onHttpRequestTimeHtml(); });
    _webServer->on("/mqtt", HTTP_GET, [&]{ onHttpRequestMqttHtml(); });
    _webServer->on("/option", HTTP_GET, [&]{ onHttpRequestOptionHtml(); });
    _webServer->on("/finish", HTTP_GET, [&]{ onHttpRequestFinishHtml(); });
    
    _webServer->on("/js/ajax.js", HTTP_GET, [&]{ onHttpRequestAjaxJs(); });
    _webServer->on("/js/app.js", HTTP_GET, [&]{ onHttpRequestAppJs(); });
    _webServer->on("/js/env.js", HTTP_GET, [&]{ onHttpRequestEnvJs(); });
    _webServer->on("/css/main.css", HTTP_GET, [&]{ onHttpRequestMainCss(); });
    
    
    
    
    _webServer->on("/api/info", HTTP_GET, [&]{ onHttpRequestInfo(); });
    
    
    _webServer->on("/api/wifi/connect", HTTP_POST, [&]{ onHttpRequestWifiConnect(); });
    _webServer->on("/api/wifi/info", HTTP_GET, [&]{ onHttpRequestSelectedSSID(); });

    _webServer->on("/api/wifi/scan/count", HTTP_GET, [&]{ onHttpRequestScanWifiCount(); });
    _webServer->on("/api/wifi/scan/item", HTTP_GET, [&]{ onHttpRequestScanWifiItem(); });


    _webServer->on("/api/ntp/info", HTTP_GET, [&]{ onHttpRequestNTPInfo(); });
    _webServer->on("/api/ntp/set", HTTP_POST, [&]{  onHttpRequestSetNTP(); });
    
    _webServer->on("/api/wifi/scan", HTTP_GET, [&]{ onHttpRequestScanWifi(); });
    _webServer->on("/api/mqtt/connect", HTTP_POST, [&]{ onHttpRequestMqttConnect(); });
    _webServer->on("/api/mqtt/info", HTTP_GET, [&]{ onHttpRequestMqttInfo(); });


    _webServer->on("/api/option/count", HTTP_GET, [&]{ onHttpRequestOptionCount(); });
    _webServer->on("/api/option/get", HTTP_GET, [&]{ onHttpRequestGetOption(); });
    _webServer->on("/api/option/set", HTTP_POST, [&]{ onHttpRequestSetOption(); });

    _webServer->on("/api/commit", HTTP_GET, [&]{ onHttpRequestCommit(); });
    
    _webServer->begin();
    //Serial.println("Setup mode started.");
}


String ESP8266ConfigurationWizard::resultStringFromEncryptionType(int thisType) {
  switch (thisType) {
    case ENC_TYPE_WEP:
      return "WEP";
      break;
    case ENC_TYPE_TKIP:
      return "WPA";
      break;
    case ENC_TYPE_CCMP:
      return "WPA2";
      break;
    case ENC_TYPE_NONE:
      return "None";
      break;
    case ENC_TYPE_AUTO:
      return "Auto";
      break;
  }
}


void ESP8266ConfigurationWizard::sendBadRequest() 
{
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
    _webServer->send(400, "text/plain", "Bad Request");
}


  void ESP8266ConfigurationWizard::onHttpRequestScanWifiCount() {
    WiFi.mode(WIFI_AP_STA);
    int n = WiFi.scanNetworks();
    //Serial.println("wifiCount : ");
    //Serial.print(n);
    //Serial.println();
    _wifiCount = n;
    String result = "";
    result.concat("{\"count\":");
    result.concat(String(n));
    result.concat("}");
    _webServer->sendHeader("Access-Control-Allow-Origin", "*");
    _webServer->send(200, "application/json", result);
}

void ESP8266ConfigurationWizard::onHttpRequestScanWifiItem() {
    String strCount =  _webServer->arg("count");
    int count = strCount.toInt();
    
    if (count >= _wifiCount) {
      _webServer->sendHeader("Access-Control-Allow-Origin", "*");
      _webServer->send(200, "application/json", "{}");
    }
    String result = "";
    result.concat("{\"ssid\":\"");
    result.concat(String(WiFi.SSID(count)));
    result.concat("\",\"type\":\"");
    result.concat(resultStringFromEncryptionType(WiFi.encryptionType(count)));
    result.concat("\",\"rssi\":");
    result.concat(String(WiFi.RSSI(count)));
    result.concat("}");

    //Serial.println(result);
    
    _webServer->sendHeader("Access-Control-Allow-Origin", "*");
    _webServer->send(200, "application/json", result);
}



void ESP8266ConfigurationWizard::onHttpRequestScanWifi() {
    WiFi.mode(WIFI_AP_STA);
    int n = WiFi.scanNetworks();
    String result = "";
    if (n <= 0) {
      _webServer->sendHeader("Access-Control-Allow-Origin", "*");
      _webServer->send(200, "application/json", "[]");
    }
    result.concat("[");
    for (int i = 0; i < n; ++i) {
      result.concat("{\"ssid\":\"");
      result.concat(String(WiFi.SSID(i)));
      result.concat("\",\"type\":\"");
      result.concat(resultStringFromEncryptionType(WiFi.encryptionType(i)));
      result.concat("\",\"rssi\":");
      result.concat(String(WiFi.RSSI(i)));
      result.concat("}");
      if(i != n - 1)  result.concat(",");
    }
    result.concat("]");
    
    _webServer->sendHeader("Access-Control-Allow-Origin", "*");
    _webServer->send(200, "application/json", result);
}



void ESP8266ConfigurationWizard::onHttpRequestWifiHtml() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/html", RES_WIFI_HTML); 
}


void ESP8266ConfigurationWizard::onHttpRequestTimeHtml() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/html", RES_TIME_HTML); 
}

void ESP8266ConfigurationWizard::onHttpRequestMqttHtml() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/html", RES_MQTT_HTML); 
}

void ESP8266ConfigurationWizard::onHttpRequestOptionHtml() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/html", RES_OPTION_HTML); 
}

void ESP8266ConfigurationWizard::onHttpRequestFinishHtml() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/html", RES_FINISH_HTML); 
}

void ESP8266ConfigurationWizard::onHttpRequestAppJs() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/javascript", RES_APP_JS); 
}

void ESP8266ConfigurationWizard::onHttpRequestEnvJs() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/javascript", RES_ENV_JS); 
}

void ESP8266ConfigurationWizard::onHttpRequestAjaxJs() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/javascript", RES_AJAX_JS); 
}

void ESP8266ConfigurationWizard::onHttpRequestMainCss() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "text/css", RES_MAIN_CSS); 
}


void ESP8266ConfigurationWizard::onHttpRequestInfo() {
  IPAddress myIP = WiFi.localIP();
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "application/json", String("{\"connected\":") +  ( (WiFi.status() != WL_CONNECTED) ? "false" : "true" )  +  ",\"ip\":\"" + myIP.toString()  + "\",\"version\":\"" + _config.version()  + "\",\"ssid\":\"" + _config.getWiFiSSID() + "\",\"device\":\"" + _config.getDeviceName() + "\"}"); 
}

void ESP8266ConfigurationWizard::onHttpRequestCommit() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  if(saveConfig()) {
      _webServer->send(200, "application/json", "{\"success\":true}");
  } else {
      _webServer->send(200, "application/json", "{\"success\":false}");
  }
  delay(1000);
  ESP.restart();
}



void ESP8266ConfigurationWizard::onHttpRequestWifiConnect()  {
  
  String ssid = _webServer->arg("ssid");
  String password = _webServer->arg("password");
  Serial.print("ssid : ");
  Serial.println(ssid);
  Serial.print("password : ");
  //Serial.println(password);
  
  if (ssid.length() <= 0) {
      sendBadRequest();
      return;
  }
  
  if(password.length() <= 0)  WiFi.begin(ssid);
  else WiFi.begin(ssid, password);
  //Serial.print("Begin wifi : ");
  
  long current = millis();
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
      delay(1);
      if(millis() - current > WIFI_TIMEOUT) {       
        //Serial.print("Begin wifi - fatal ");
        _webServer->sendHeader("Access-Control-Allow-Origin", "*");
        _webServer->send(400, "application/json", "{\"success\":false}");
        return;
      }
  }
  //Serial.print("Begin wifi - success");
  _config.setWiFiSSID(ssid);
  _config.setWiFiPassword(password);
  
  
  
  IPAddress myIP = WiFi.localIP();
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  return _webServer->send(200, "application/json", String("{\"success\":true,\"ip\":\"") + myIP.toString()  + "\"}");
}



void ESP8266ConfigurationWizard::onHttpRequestMqttConnect()  {
    String server = _webServer->arg("url");
    String portStr = _webServer->arg("port");
    String user = _webServer->arg("muser");
    String pass = _webServer->arg("mpass");
    String clientID = _webServer->arg("mid");

    int port = portStr.toInt();
    //Serial.println(server);
    //Serial.println(port);
    bool connected = false; 
    if(port <= 0) port = 1883;


    //Serial.print("user: ");
    //Serial.print(user);
    //Serial.println(".");
    //Serial.print("pass: ");
    //Serial.print(pass);
    //Serial.println(".");

    if(_mqtt->connected()) {
      _mqtt->disconnect();
    }
    _mqtt->setServer(server.c_str(), port);
    if (user.isEmpty() && pass.isEmpty() ) {
      if(_mqtt->connect(clientID.c_str())) {
        connected = true;
        _config.setMQTTddress(server);
        _config.setMQTTPort(port);
        _config.setMQTTClientID(clientID);
        _config.setMQTTUser("");
        _config.setMQTTPassword("");
      }
    }
    else if(_mqtt->connect(clientID.c_str(), user.c_str(), pass.c_str())) {
      connected = true;
      _config.setMQTTddress(server);
      _config.setMQTTPort(port);
      _config.setMQTTClientID(clientID);
      _config.setMQTTUser(user);
      _config.setMQTTPassword(pass);
    } 

    _webServer->sendHeader("Access-Control-Allow-Origin", "*");
    _webServer->send(200, "application/json", String("{\"success\":") + (connected ? "true" : "false")  + "}");
}


  void ESP8266ConfigurationWizard::onHttpRequestMqttInfo()  {
    _webServer->sendHeader("Access-Control-Allow-Origin", "*");
    _webServer->send(200, "application/json", String("{\"success\":true, \"url\":\"") +  _config.getMQTTAddress() + "\", \"port\":" + _config.getMQTTPort() +  ",\"muser\":\"" + _config.getMQTTUser() + "\",\"mpass\":\"" + _config.getMQTTPassword() + "\",\"mid\":\"" + _config.getMQTTClientID() +  "\"  }");
}




void ESP8266ConfigurationWizard::onHttpRequestSelectedSSID() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "application/json", String("{\"success\":true, \"ssid\":\"") +  _config.getWiFiSSID() + "\"}" );
}






void ESP8266ConfigurationWizard::onHttpRequestNTPInfo() {
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200, "application/json", String("{\"success\":true, \"ntp\":\"") +  _config.getNTPServer() + "\", \"interval\":" + _config.getNTPUpdateInterval() +  ",\"offset\":" + _config.getTimeOffset() + "}");
}

void ESP8266ConfigurationWizard::onHttpRequestSetNTP() {
  String ntpServer = _webServer->arg("ntp");
  String strTimeOffset = _webServer->arg("offset");
  long interval = atol(_webServer->arg("interval").c_str());
  if(ntpServer.isEmpty() || strTimeOffset.isEmpty() || interval == 0) {
    _webServer->send(400,"application/json", String("{\"success\":false}"));
    return;
  }
  long timeOffset = atol(strTimeOffset.c_str());
  bool isSuccess = connectNTP(ntpServer.c_str(), timeOffset, interval * 60000L);
  
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  
  if(isSuccess) {
    _config.setNTPServer(ntpServer);
    _config.setTimeOffset(timeOffset);
    _config.setNTPUpdateInterval((uint16_t)interval);
    _webServer->send(200,"application/json", String("{\"success\":true, \"h\":" + String(_ntpClient->getHours()) + ",\"m\":" + String(_ntpClient->getMinutes()) +  ",\"s\":" +  String(_ntpClient->getSeconds())  + "}"));
  } else {
    _webServer->send(400,"application/json", String("{\"success\":false}"));
  }
}


void ESP8266ConfigurationWizard::onHttpRequestOptionCount() {
  int count = _config.getOptionCount();
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _config.beginOption();
  _webServer->send(200,"application/json", String("{\"success\":true, \"cnt\":" + String(count) + "}"));
}

void ESP8266ConfigurationWizard::onHttpRequestSetOption() {
  String name = _webServer->arg("name");
  String value = _webServer->arg("value");
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  if(_onFilterOption != NULL) {
    const char* message = _onFilterOption(name.c_str(), value.c_str());
    if(message != NULL && strlen(message) > 0) {
      _webServer->send(200,"application/json", String("{\"success\":false,\"msg\":\"" + String(message) + "\" }"));
      return;
    }
  }
  if(_config.setOptionValue(name,value)) {
    _webServer->send(200,"application/json", String("{\"success\":true}"));
    return;
  }
  _webServer->send(200,"application/json", String("{\"success\":false}"));
}

void ESP8266ConfigurationWizard::onHttpRequestGetOption() {
   UserOption* option = _config.nextOption();
   const char* value = option == NULL ? "" : option->getValue();
   value = strlen(value) == 0 && option != NULL ? option->getDefaultValue() : value;
   String name =  option == NULL ? "" : option->getName();
  _webServer->sendHeader("Access-Control-Allow-Origin", "*");
  _webServer->send(200,"application/json", String("{\"success\":true, \"name\": \"" + name + "\",\"value\":\"" + String(value) + "\",\"isNull\":" + (option != NULL && option->isNull() ? "true" : "false") + "}"));
}


bool ESP8266ConfigurationWizard::saveConfig() {
      if(!LittleFS.begin()){
        return false;
      }
       

      char NTPUpdateIntervalBuf[16];
      char offsetBuf[16];
      char mqttPortBuf[16];
      memset(NTPUpdateIntervalBuf, '\0', 16);
      memset(offsetBuf, '\0', 16);
      memset(mqttPortBuf, '\0', 16);
      ltoa(_config.getNTPUpdateInterval(), NTPUpdateIntervalBuf,10);
      ltoa(_config.getTimeOffset(), offsetBuf,10);
      ltoa(_config.getMQTTPort(), mqttPortBuf,10);

       DynamicJsonDocument doc(CONFIG_FILE_SIZE);        
       doc["version"] = _config.version();
       doc["deviceName"] = _config.getDeviceName();
       doc["wifi"]["ssid"] = _config.getWiFiSSID();
       doc["wifi"]["password"] = _config.getWiFiPassword();

       doc["ntp"]["address"] = _config.getNTPServer();

       
       doc["ntp"]["interval"] = NTPUpdateIntervalBuf;
       doc["ntp"]["offset"] = offsetBuf;

       doc["mqtt"]["address"] = _config.getMQTTAddress();
       doc["mqtt"]["port"] = mqttPortBuf;
       doc["mqtt"]["clientID"] = _config.getMQTTClientID();
       doc["mqtt"]["user"] = _config.getMQTTUser();
       doc["mqtt"]["password"] = _config.getMQTTPassword();

       LittleFS.remove("/config001.json");
       LittleFS.remove(CONFIG_FILENAME);
       
       File configFile = LittleFS.open(CONFIG_FILENAME, "w");
       if (!configFile) {
         Serial.println("Failed to open config file for writing");
         LittleFS.end();
         return false;
       }
       serializeJson(doc, configFile);
       configFile.close();

       _availableConfigFile = true;
       saveConfigOptions();
       LittleFS.end();
       return true;
    }


    bool ESP8266ConfigurationWizard::saveConfigOptions() {
      
	Serial.println("sAVE");
      LittleFS.remove(OLD_OPTIONS_FILENAME);
      LittleFS.remove(OPTIONS_FILENAME);
      File optionsFile = LittleFS.open(OPTIONS_FILENAME, "w");
      if (!optionsFile) {
        Serial.println("Failed to open config file for writing");
        return false;
      }
      _config.beginOption();
      for(int i = 0, n = _config.getOptionCount(); i < n; ++i) {
        UserOption* option = _config.nextOption();
        String name = option->getName();
        String value = option->getValue();
        optionsFile.write(name.c_str(), name.length());
        optionsFile.write("\n",1);
        optionsFile.write(value.c_str(),  value.length());
        optionsFile.write("\n",1);
      }
      Serial.print("option file size: ");
      Serial.println(optionsFile.size());
      optionsFile.close();
      return true;
    }


    bool ESP8266ConfigurationWizard::loadConfigOptions() {
      /*if(!LittleFS.begin()){
         Serial.println("An Error has occurred while mounting LittleFS");
         return false;
      }*/
      Serial.println("Load options file");
      Serial.println(OPTIONS_FILENAME);
      File optionsFile = LittleFS.open(OPTIONS_FILENAME, "r");
      if (!optionsFile) {
        Serial.println("Failed to open options file");
        //LittleFS.end();
        return false;
      }

      Serial.print("\nload option file size: ");
      Serial.println(optionsFile.size());

      //_config.clearOptions();
      char nameBuffer[512];      
      char valueBuffer[512];      
      memset(nameBuffer, '\0', 512);
      memset(valueBuffer, '\0', 512);
      int cnt = 0;
      bool isReadName = true; 
      //_config.beginOption();
      while(optionsFile.available()){
        char ch = optionsFile.read();
        if(ch != '\n') {
          if(isReadName == true) nameBuffer[cnt++] = ch;
          else valueBuffer[cnt++] = ch;
        }
        else if(ch == '\n') {
          if(!isReadName) {
           _config.addOption(String(nameBuffer), "", true);
           _config.setOptionValue(String(nameBuffer), String(valueBuffer));
           //Serial.print(String(nameBuffer));
           //Serial.print(':');
           //Serial.println(String(valueBuffer));
           memset(nameBuffer, '\0', 512);
           memset(valueBuffer, '\0', 512);
          }
          
          cnt = 0;
          isReadName = !isReadName;
        }
      }
      Serial.println("CloseCloseCloseCloseClose");
      optionsFile.close();
      Serial.println("endendendend");
      //LittleFS.end();
      Serial.println("endendendend...OK!");

    }
    
                
    bool ESP8266ConfigurationWizard::loadConfig() {
      if(!LittleFS.begin()){
         Serial.println("An Error has occurred while mounting LittleFS");
         return false;
      }
      Serial.println("Load file");
      Serial.println(CONFIG_FILENAME);
	  File configFile = LittleFS.open(CONFIG_FILENAME, "r");
	  if (!configFile) {
        Serial.println("Failed to open config file");
        _availableConfigFile = false;
        LittleFS.end();
        return false;
      }
	  size_t size = configFile.size();
	  if (size > CONFIG_FILE_SIZE) {
        Serial.println("Config file size is too large");
        _availableConfigFile = false;
        LittleFS.end();
        return false;
      }
      //std::unique_ptr<char[]> buf(new char[size]);
      //configFile.readBytes(buf.get(), size);

		Serial.println("Load file -5");
      DynamicJsonDocument doc(CONFIG_FILE_SIZE);

      //Serial.println(buf.get());
      

      auto error = deserializeJson(doc, configFile);
      if (error) {
        Serial.println("Failed to parse config file");
        return false;
      }

       _config.setVersion(doc["version"]);
       _config.setDeviceName(doc["deviceName"]);
       _config.setWiFiSSID(doc["wifi"]["ssid"]);
       _config.setWiFiPassword(doc["wifi"]["password"]);

       _config.setNTPServer(doc["ntp"]["address"]);
       _config.setNTPUpdateInterval(atol(doc["ntp"]["interval"]) );
       _config.setTimeOffset(atol(doc["ntp"]["offset"]));

       _config.setMQTTddress( doc["mqtt"]["address"] );
       _config.setMQTTPort(atoi(  doc["mqtt"]["port"] ));
       _config.setMQTTClientID(  doc["mqtt"]["clientID"] );
       _config.setMQTTUser( doc["mqtt"]["user"]  );
       _config.setMQTTPassword( doc["mqtt"]["password"] );


       _config.printConfig();

       configFile.close();
      _availableConfigFile = true;
      loadConfigOptions();
      LittleFS.end();
      return true;
    }

