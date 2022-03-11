#pragma once 
#include "LinkedList.hpp"
#include "UserOption.hpp"

#define CONFIG_OFFSET 64
#define BUFF_SIZE 64


class Config {
 private : 
  String _version;
  String _deviceName;

  String _apName;
  
  String _ssid; // ssid, 공유기의 이름
  String _pass; // ap password, 공유기 접속 비번.
 
  String _mqttAddress; // MQTT adderess
  int _mqttPort; // MQTT port
  String _mqttClientID;
  String _mqttUser;
  String _mqttPass;
  
  String _ntpServer;
  long _timeOffset;
  uint16_t _ntpUpdateInterval;
  LinkedList<UserOption*> *_userOptionList;


  public : 
    Config();
    Config(const Config& conf);
    ~Config();
    Config& operator=(const Config& conf);
    void setAPName(String name);
    const char* getAPName();
    const char* version();
    void setVersion(String version);
    const char* getNTPServer();
    void setNTPServer(String serverAddr);
    void setTimeOffset(long timeOffset);
    void setTimeZone(long hour);
    long getTimeOffset();
    void setNTPUpdateInterval(uint16_t value);
    uint16_t getNTPUpdateInterval();
    const char*  getWiFiPassword();
    const char* getWiFiSSID();
    const char* getDeviceName();
    void setDeviceName(String deviceName);
    void setWiFiPassword(String pass);
    void setWiFiSSID(String ssid);
    const char* getMQTTAddress();
    void setMQTTddress(String address);
    const char* getMQTTClientID();
    void setMQTTClientID(String clientID);
    void setMQTTUser(String mqttUser);
    const char* getMQTTUser();
    void setMQTTPassword(String mqttPass);
    const char* getMQTTPassword();
    UserOption* addOption(String name, String defaultValue,bool isNull);
    bool setOptionValue(String name, String value);
    int getOptionCount() const;
    void beginOption() const;
    UserOption* nextOption() const;
    bool hasMoreOption() const;
    int getMQTTPort();
    void setMQTTPort(uint16_t port);
    void printConfig();
    void clearOptions();

    private: 

    void copy(const Config& conf);
    UserOption* findOption(String name);
      

};



Config::Config() : _ssid(""), _pass(""), _mqttAddress(""),_ntpServer("")   {
  _userOptionList = new LinkedList<UserOption*>();
  _version = "0.0.0";
  _deviceName = "My Device";

  _apName = "ESP8266 Configuration";
  
  _ssid = ""; // ssid, 공유기의 이름
  _pass = ""; // ap password, 공유기 접속 비번.
 
  _mqttAddress = "broker.hivemq.com"; // MQTT adderess
  _mqttPort = 1883; // MQTT port
  _mqttClientID = "";
  _mqttUser = "";
  _mqttPass = "";

  _ntpServer = "time.google.com";
  _timeOffset = 0;
  _ntpUpdateInterval = 1440;
}

Config::Config(const Config& conf) {
    _userOptionList = new LinkedList<UserOption*>();
    copy(conf);
}

Config::~Config() {
  Serial.println("~Config");
  clearOptions();
  delete _userOptionList;
}

Config& Config::operator=(const Config& conf) {
  clearOptions();
  copy(conf);
  return *this;
}

void Config::setAPName(String name) {
  _apName = name;
}

const char* Config::getAPName() {
  return _apName.c_str();
}

void Config::setVersion(String version)  {
  _version = version;
}

const char* Config::version()  {
  return _version.c_str();
}

const char* Config::getNTPServer() {
  return _ntpServer.c_str();
}

void Config::setNTPServer(String serverAddr) {
    _ntpServer = serverAddr;
}

void Config::setTimeOffset(long timeOffset) {
    _timeOffset = timeOffset;
}

void Config::setTimeZone(long hour) {
    _timeOffset = hour * 60 * 60;
}

long Config::getTimeOffset() {
    return _timeOffset;
}

void Config::setNTPUpdateInterval(uint16_t value) {
  _ntpUpdateInterval = value;
}

uint16_t Config::getNTPUpdateInterval() {
  return _ntpUpdateInterval;
}

const char*  Config::getWiFiPassword()  {
  return _pass.c_str();
}

const char* Config::getWiFiSSID()  {
  return _ssid.c_str();
}

const char* Config::getDeviceName() {
    return _deviceName.c_str();
}

void Config::setDeviceName(String deviceName) {
    _deviceName = deviceName;
}


void Config::setWiFiPassword(String pass)  {
  _pass = pass;
}

void Config::setWiFiSSID(String ssid)  {
  _ssid = ssid;
}

const char* Config::getMQTTAddress() {
  return _mqttAddress.c_str();
}

void Config::setMQTTddress(String address) {
  address.trim();
  _mqttAddress = address;
}


const char* Config::getMQTTClientID() {
  return _mqttClientID.c_str();
}

void Config::setMQTTClientID(String clientID) {
  _mqttClientID = clientID;
}

void Config::setMQTTUser(String mqttUser) {
  mqttUser.trim();
  _mqttUser = mqttUser;
}


const char* Config::getMQTTUser() {
  return _mqttUser.c_str();
}

void Config::setMQTTPassword(String mqttPass) {
  _mqttAddress.trim();
  _mqttPass = mqttPass;
}

const char* Config::getMQTTPassword() {
  return _mqttPass.c_str();
}

UserOption* Config::addOption(String name, String defaultValue,bool isNull) {
  Serial.println("Config::addOption_1");
  UserOption* userOption = findOption(name);
  Serial.println("Config::addOption_2");
  if(userOption == NULL) {
    Serial.println("Config::addOption_3");
      userOption = new UserOption(name, defaultValue, isNull);
      Serial.println("Config::addOption_4");
      _userOptionList->Append(userOption);
      Serial.println("Config::addOption_5");
  } else {
    Serial.println("Config::addOption_6");
    userOption->setDefaultValue(defaultValue);
    Serial.println("Config::addOption_7");
    userOption->setIsNull(isNull);
    Serial.println("Config::addOption_8");
  }
  return userOption;
}

bool Config::setOptionValue(String name, String value) {
  UserOption* option = findOption(name);
  Serial.print(name);
  Serial.print(": ");
  Serial.println(value);
  if(option == NULL) {
    Serial.println("option is NULL");
  }

  if(option == NULL || (value.isEmpty() && !option->isNull()) ) {
    return false;
  }
  option->setValue(value);
  return true; 
}

int Config::getOptionCount() const  {
  return _userOptionList->getLength();
}


void Config::beginOption() const  {
  _userOptionList->moveToStart();
}

UserOption* Config::nextOption() const {
  if(_userOptionList->getLength() == 0) return NULL;
  UserOption* option = _userOptionList->getCurrent();
  _userOptionList->next();
  
  return option;
}


void Config::printConfig() {
    Serial.print("version: ");
    Serial.println(_version);
    Serial.print("device name: ");
    Serial.println(_deviceName);
    Serial.print("ap name: ");
    Serial.println(_apName);

    Serial.println();
    Serial.println("::WIFI::");
    Serial.print("    ssid: ");
    Serial.println(_ssid);
    Serial.print("    ssid pass: ");
    Serial.println(_pass);
    
    Serial.println();
    Serial.println("::NTP::");
    Serial.print("    address: ");
    Serial.println(_ntpServer);
    Serial.print("    update interval: ");
    Serial.println(_ntpUpdateInterval);
    Serial.print("    Time offset: ");
    Serial.println(_timeOffset);;;


    Serial.println();
    Serial.println("::MQTT::");
    Serial.print("    address: ");
    Serial.println(_mqttAddress);
    Serial.print("    port: ");
    Serial.println(_mqttPort);
    Serial.print("    clientID: ");
    Serial.println(_mqttClientID);
    Serial.print("    user: ");
    Serial.println(_mqttUser);
    Serial.print("    password: ");
    Serial.println(_mqttPass);


    Serial.println();
    Serial.println("::Options::");
   
    beginOption();
    for(int i = 0, n = getOptionCount(); i < n; ++i) {        
        UserOption* userOption = nextOption();
        Serial.print("    ");
        Serial.print(userOption->getName());
        Serial.print(": ");
        Serial.print(userOption->getValue());
        Serial.println();
    }
}


int Config::getMQTTPort() {
  return _mqttPort;
}

void Config::setMQTTPort(uint16_t port) {
  _mqttPort = port;
}


void Config::copy(const Config& conf) {
    _version = conf._version;
    _deviceName = conf._deviceName;
    _apName = conf._apName;
    _ssid = conf._ssid;
    _pass = conf._pass;
    _mqttAddress = conf._mqttAddress;
    _mqttPort = conf._mqttPort;
    _mqttClientID = conf._mqttClientID;
    _mqttUser = conf._mqttUser;
    _mqttPass = conf._mqttPass;
    _ntpServer = conf._ntpServer;
    _timeOffset = conf._timeOffset;
    _ntpUpdateInterval = conf._ntpUpdateInterval;

    conf.beginOption();
    for(int i = 0, n = conf.getOptionCount(); i < n; ++i) {        
        UserOption* userOption = conf.nextOption();
        UserOption* cpyUserOption = new UserOption(userOption->getName(), userOption->getDefaultValue(), userOption->isNull()); 
        cpyUserOption->setValue(userOption->getValue());
        _userOptionList->Append(cpyUserOption);
    }
}

void Config::clearOptions() {
    if(_userOptionList->getLength() == 0) return;
    _userOptionList->moveToStart();
    for(int i = 0, n = getOptionCount(); i < n; ++i) {
        UserOption* userOption = nextOption();
        delete userOption;
    }
    _userOptionList->Clear();
    Serial.println("end config");
}


UserOption* Config::findOption(String name) {
  if(_userOptionList->getLength() == 0) return NULL;
    _userOptionList->moveToStart();
    for(int i = 0, n = getOptionCount(); i < n; ++i) {
        UserOption* userOption =  nextOption();
        if(strcmp(name.c_str(), userOption->getName()) == 0) {
          return userOption;
        }  
    }
    return NULL;
}

