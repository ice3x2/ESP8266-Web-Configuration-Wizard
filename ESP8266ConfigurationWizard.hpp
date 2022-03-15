#ifndef ESPCONFIGURATIONWIZARD.HPP
#define ESPCONFIGURATIONWIZARD.HPP


#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <WifiServer.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include "Config.hpp"
#include "Resources.hpp"
#include "LinkedList.hpp"

// PubSubClient >= 2.8.0

#define ESP_CONFIGURATION_WIZARD_VERSION "0.9.0\0"

#define CONFIG_FILENAME "/config.dat"

#define VALUE_BUFFER_SIZE 512


#define MQTT_RECONNECT_INTERVAL 3000
#define MQTT_SOCKET_TIMEOUT 3000
#define MQTT_KEPP_ALIVE 1000

#define NTP_RECONNECT_INTERVAL 500



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

#define STATUS_CONFIGURATION 7

class ESP8266ConfigurationWizard {

  private :

    WiFiUDP _udp;
    WiFiClient _wifiClient; 
    ESP8266WebServer* _webServer;
    NTPClient* _ntpClient;
	PubSubClient _mqtt;
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
    


 public:
    ESP8266ConfigurationWizard();
    void setOnFilterOption(option_filter filter);
    void setOnStatusCallback(status_callback callback);
    Config& getConfig();
    Config* getConfigPt();
    void setConfig(Config config);
    bool available();
    bool availableWifi();
    bool availableNTP();
    bool availableMqtt();
    void connect();
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
  void writeLineInConfigFile(File* file,const char* value);
  char* readLineInConfigFile(File* file,char* buffer);
  
  bool saveConfigOptions(File* file);
  bool loadConfigOptions(File* file,char* buffer);
  

};






ESP8266ConfigurationWizard::ESP8266ConfigurationWizard() : _webServer(NULL), _ntpClient(NULL)
{
	_mqtt.setClient(_wifiClient);
  
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
  return _mqtt.connected();
}

bool ESP8266ConfigurationWizard::availableWifi() {
  if(WiFi.status() == WL_CONNECTED) {
    return true;  
  }
  return false; 
}


void ESP8266ConfigurationWizard::connect() {
  
  if(!loadConfig()) {
    startConfigurationMode();
    return;
  }
  
  releaseWebServer();
  _mode = MODE_RUN;
  _mqtt.setSocketTimeout(MQTT_SOCKET_TIMEOUT);
  _mqtt.setKeepAlive(MQTT_KEEPALIVE);

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
	delay(100);
  }
  setStatus(MQTT_CONNECTED); 
  setStatus(STATUS_OK);
  
}

void ESP8266ConfigurationWizard::startConfigurationMode() {
  _mode = MODE_CONFIGURATION;
  setStatus(STATUS_CONFIGURATION);
  initConfigurationMode();
}

bool ESP8266ConfigurationWizard::isConfigurationMode() {
  return _mode == MODE_CONFIGURATION;
}

PubSubClient* ESP8266ConfigurationWizard::pubSubClient() {
    return &_mqtt;
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

	 if(_mqtt.connected()) {
		_mqtt.loop();
	 }
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
    
    _mqtt.setServer(server,port);     
    if(!_mqtt.connected()) {
        #ifdef _DEBUG_
			Serial.print("connect mqtt: ");
			Serial.println(id);
			Serial.print("mqtt addr: ");
			Serial.println(_config.getMQTTAddress());
			Serial.print("mqtt port: ");
			Serial.println(_config.getMQTTPort());
			Serial.print("mqtt clinetID: ");
			Serial.println(_config.getMQTTClientID());
			Serial.print("mqtt user: ");
			Serial.println(_config.getMQTTUser());
			Serial.print("mqtt password: ");
			Serial.println(_config.getMQTTPassword());           
			Serial.println(strlen(_config.getMQTTUser()));
        #endif
		
        if(strlen(_config.getMQTTUser()) > 0 && (_mqtt.connect(id,user, password) || _mqtt.connected())) {
            #ifdef _DEBUG_ 
			Serial.println("mqtt connected(user)");
			#endif
            return true;
        } else if (_mqtt.connect(id) || _mqtt.connected()) {             
			#ifdef _DEBUG_
			Serial.println("mqtt connected(id)");
			#endif
            return true;
        }
        else {
          #ifdef _DEBUG_ 
          Serial.println("failed connect mqtt");
		  #endif
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
    
    
    long startConnectTime = millis();
    _ntpClient->update();
    if(!_ntpClient->isTimeSet()) {
      delay(NTP_RECONNECT_INTERVAL);
      _ntpClient->setRandomPort();
      _ntpClient->update();
    }
    
    
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
	#ifdef _DEBUG_ 
	Serial.println("initConfigurationMode()");
	#endif
	if(_ntpClient != NULL) {
        _ntpClient->end();
        delete _ntpClient;
        _ntpClient = NULL;
    }
    releaseWebServer();
    _webServer = new ESP8266WebServer(80);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(_config.getAPName(), "");
    
    IPAddress myIP = WiFi.softAPIP();
	#ifdef _DEBUG_ 
    Serial.print("AP IP address: ");
    Serial.println(myIP);
	#endif

    
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
  #ifdef _DEBUG_ 
  Serial.print("ssid : ");
  Serial.println(ssid);
  Serial.print("password : ");
  #endif
  
  if (ssid.length() <= 0) {
      sendBadRequest();
      return;
  }
  
  if(password.length() <= 0)  WiFi.begin(ssid);
  else WiFi.begin(ssid, password);
  
  
  long current = millis();
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
      delay(1);
      if(millis() - current > WIFI_TIMEOUT) {       
  
        _webServer->sendHeader("Access-Control-Allow-Origin", "*");
        _webServer->send(400, "application/json", "{\"success\":false}");
        return;
      }
  }
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
    bool connected = false; 
    if(port <= 0) port = 1883;


    if(_mqtt.connected()) {
      _mqtt.disconnect();
    }
    _mqtt.setServer(server.c_str(), port);
    if (user.isEmpty() && pass.isEmpty() ) {
      if(_mqtt.connect(clientID.c_str())) {
        connected = true;
        _config.setMQTTddress(server);
        _config.setMQTTPort(port);
        _config.setMQTTClientID(clientID);
        _config.setMQTTUser("");
        _config.setMQTTPassword("");
      }
    }
    else if(_mqtt.connect(clientID.c_str(), user.c_str(), pass.c_str())) {
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


		LittleFS.remove("/config002.dat");
		LittleFS.remove(CONFIG_FILENAME);
		File configFile = LittleFS.open(CONFIG_FILENAME, "w");
		if (!configFile) {
			#ifdef _DEBUG_ 
			Serial.println("Failed to open config file for writing");
			#endif
			return false;
		}	  
		
		writeLineInConfigFile(&configFile,ESP_CONFIGURATION_WIZARD_VERSION);

		writeLineInConfigFile(&configFile,_config.version());	  
		writeLineInConfigFile(&configFile,_config.getDeviceName());
		writeLineInConfigFile(&configFile,_config.getWiFiSSID());	  
		writeLineInConfigFile(&configFile,_config.getWiFiPassword());	  
		writeLineInConfigFile(&configFile,_config.getNTPServer());	  
		writeLineInConfigFile(&configFile,NTPUpdateIntervalBuf);	  
		writeLineInConfigFile(&configFile,offsetBuf);	  
		writeLineInConfigFile(&configFile,_config.getMQTTAddress());	  
		writeLineInConfigFile(&configFile,mqttPortBuf);	  
		writeLineInConfigFile(&configFile,_config.getMQTTClientID());	  
		writeLineInConfigFile(&configFile,_config.getMQTTUser());	  
		writeLineInConfigFile(&configFile,_config.getMQTTPassword());	

		if(!saveConfigOptions(&configFile)){
			configFile.close();
			return false;
		}

		configFile.close();


		return true;
    }
	
	
	bool ESP8266ConfigurationWizard::saveConfigOptions(File* file) {    
		int optionCount=_config.getOptionCount();
		char optionCountBuffer[16];
		memset(optionCountBuffer, '\0', 16);
		itoa(optionCount, optionCountBuffer,10);
		writeLineInConfigFile(file,optionCountBuffer);

		_config.beginOption();
		for(int i = 0, n = optionCount; i < n; ++i) {
			UserOption* option = _config.nextOption();
			String name = option->getName();
			String value = option->getValue();
			writeLineInConfigFile(file,name.c_str());
			writeLineInConfigFile(file,value.c_str());
		}
		return true;
    }
	
	
	void ESP8266ConfigurationWizard::writeLineInConfigFile(File* file, const char* value) {
		file->write(value, strlen(value));
		file->write("\n", 1);
	}
	
	
	 
    bool ESP8266ConfigurationWizard::loadConfig() {
		if(!LittleFS.begin()){
			return false;
		}

		char buffer[VALUE_BUFFER_SIZE];      
		memset(buffer, '\0', VALUE_BUFFER_SIZE);
		#ifdef _DEBUG_ 
		Serial.println("Load file");
		Serial.println(CONFIG_FILENAME);
		#endif
		File configFile = LittleFS.open(CONFIG_FILENAME, "r");
		

		if (!configFile) {
			#ifdef _DEBUG_ 
			Serial.println("Failed to open config file");
			#endif

			return false;
		}
		char* value = NULL;
		// 라인이 null 값이면 정리하고 false 를 반홚야함.
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		if(strcmp(value, ESP_CONFIGURATION_WIZARD_VERSION) != 0) {
			configFile.close();
			return false;
		}
		
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setVersion(String(value));
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setDeviceName(String(value));
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setWiFiSSID(String(value));
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setWiFiPassword(String(value));

		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setNTPServer(String(value));
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setNTPUpdateInterval(atol(value));
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setTimeOffset(atol(value));

		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setMQTTddress( String(value) );
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setMQTTPort(atoi(  value ));
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setMQTTClientID(  String(value) );
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setMQTTUser( String(value)  );
		if((value = readLineInConfigFile(&configFile, buffer)) == NULL) {
			configFile.close();
			return false;
		}
		_config.setMQTTPassword( String(value) );


		if(!loadConfigOptions(&configFile, buffer)) {
			configFile.close();
		   return false;
		}

		configFile.close();

		return true;
    }
	
	bool ESP8266ConfigurationWizard::loadConfigOptions(File *file, char* buffer) {
		
	  bool isReadName = true; 
	  
	  char* value = NULL;
   	  // 라인이 null 값이면 정리하고 false 를 반홚야함.
	  if((value = readLineInConfigFile(file, buffer)) == NULL) return false;
	  int optionCount = atoi(  value );
	  
      for(int i = 0; i < optionCount; ++i) {
		if((value = readLineInConfigFile(file, buffer)) == NULL) return false;
		String name(value);
		if((value = readLineInConfigFile(file, buffer)) == NULL) return false;		
		String optionValue(value);
		#ifdef _DEBUG_
		Serial.print(name);
		Serial.print(":");
		Serial.println(optionValue);
		#endif
		_config.setOptionValue(name, optionValue);
      }
	  #ifdef _DEBUG_
      Serial.println("loadConfigOptions()...OK!");
	  #endif
	  return true;

    }

	char* ESP8266ConfigurationWizard::readLineInConfigFile(File* file, char* buffer) {
		memset(buffer, '\0', VALUE_BUFFER_SIZE);
		int cnt = 0;
		while(file->available()){
			char ch = file->read();
			if(ch != '\n') {
			  buffer[cnt++] = ch;
			} else {				
			  return buffer;
			}
		}
		return NULL;
	}


    
    
#endif
