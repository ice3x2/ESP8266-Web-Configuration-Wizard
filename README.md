# ESP8266 Web Configuration Wizard
 웹을 통해 ESP8266의 WiFi 연결, NTP, MQTT 및 옵션을 구성할 수 있는 마법사 도구입니다.
 This is a wizard tool that allows you to configure the ESP8266's WiFi connection, NTP, MQTT, and Options via web.
 
![screen](https://user-images.githubusercontent.com/3121298/159404924-a39ab1b0-27f6-430b-9b0e-cbb3a15bd1f5.png)

## 의존성 (Dependency)
  * PubSubClient >= 2.8.0
  * NTPClient >= 3.2.0 (반드시 이 곳에서 다운로드 https://github.com/arduino-libraries/NTPClient)

## 라이브러리 적용방법
  1. https://github.com/arduino-libraries/NTPClient 로 접속하여 프로젝트를 다운로드 받아 압축을 풀고 아두이노의 라이브러리 디렉토리 (윈도우의 경우 "Documents\Arduino\libraries") 에 폴더채로 넣습니다.
  2. 이 프로젝트를 다운로드 받아 압축을 풀고 아두이노의 라이브러리 디렉토리에 폴더채로 넣습니다. 

## 사용방법
  * [샘플 코드](https://github.com/ice3x2/ESP8266-Web-Configuration-Wizard/blob/master/examples/sample/sample.ino)
### 초기화
  ```cpp
  #include "ESP8266ConfigurationWizard.hpp"

  PubSubClient* _mqttClinet;
  ESP8266ConfigurationWizard _ESP8266ConfigurationWizard;
  
  void setup() {
   Config* config = _ESP8266ConfigurationWizard.getConfigPt();
   // 무선 AP 이름 
   config->setAPName("ESP8266 Wizard");
   // 디바이스 이름. 아직 특별히 사용되지는 않는다.
   // config->setDeviceName("");
   
   // 설정된 기본값들은 설정 페이지 Input 박스에 기본값으로 표시됩니다. 
   
   // NTP 기본값 설정
   // 기본 NTP 서버 주소
   config->setNTPServer("time.google.com");
   // 기본 시간대. 서울은 +9
   config->setTimeZone(9);
   // 시간대가 1시간 단위가 아닌 곳은 분 단위로 설정 가능. 
   // config->setTimeOffset(-1000);
   // 분단위로 설정되는 NTP 서버 업데이트 간격. (1440분 = 24시간)
   config->setNTPUpdateInterval(1440);
   
   // MQTT 기본값 설정
   // 기본 MQTT 서버 주소 
   config->setMQTTddress("broker.hivemq.com");
   // 기본 MQTT 포트. 기본값 1883
   // config->setMQTTPort(1883);
   // 설정하지 않을경우 23자의 랜덤 문자열이 입력된다.
   // 굳이 기본값을 설정하지 않는 것을 권장.
   // config->setMQTTClientID("client_id");
   // MQTT 기본 접속 정보 설정 
   // config->setMQTTUser("");
   // config->setMQTTPassword("");
   
   // MQTT 클라이언트 객체 가져오기. 
   _mqttClinet = _ESP8266ConfigurationWizard.pubSubClient();
   
   // ... 생략 ...
   
```
### 옵션 추가
  * 사용자가 옵션 설정 페이지를 통하여 직접 입력 가능한 옵션을 정의할 수 있습니다. 
  * 옵션 필터링을 통하여 올바른 값을 입력할 수 있도록 유도할 수 있습니다. 
```cpp
 //.. 생략...  
 
 const char* onFilterOption(const char* name, const char* value);
 
 void setup() {
   
   Config* config = _ESP8266ConfigurationWizard.getConfigPt();
   // ...생략... 
   // 이 곳에 초기화 코드.
   
   // 첫 번째 인자: 옵션 키
   // 두 번째 인자: 기본 값. 
   // 세 번째 인자: 빈 값 허용 
   config->addOption("DeviceName","", false);
   config->addOption("UserName","User", true);
   
   
   
   // 옵션 필터 추가. 
   _ESP8266ConfigurationWizard.setOnFilterOption(onFilterOption);
   // ... 생략 ... 
 }

// 옵션 필터링
const char* onFilterOption(const char* name, const char* value) {
  // 옵션 값이 16자를 초과하면 에러 메시지 출력. 
  int valueLen = strlen(value);
  if(valueLen > 16) {
     return "Please enter 16 characters or less.";
  }
  
  // 옵션 키 값이 "DeviceName"이라면 영문, 숫자만 입력 가능. 
  if(strcmp(name, "DeviceName") == 0) {
      for(int i = 0; i < valueLen; ++i) {
        if( !(('0' <= value[i] && value[i] <= '9') || 
              ('A' <= value[i] && value[i] <= 'Z') || 
              ('a' <= value[i] && value[i] <= 'z') ) ) 
        {
          return "Only numbers or English alphabets can be entered.";
        }
      }
  }
  // 빈 문자열을 반환하면 필터링되지 않은 정상값. 
  return "";
}
```
 * 유저가 입력한 옵션은 connect() 함수 호출 이후에 가져올 수 있으며, 아래와 같은 방법으로 얻을 수 있습니다.
```cpp
  Config* config = _ESP8266ConfigurationWizard.getConfigPt();
  String deviceKey = config->getOption("deviceKey");
```
### 연결
```cpp
 // ... 생략 ...
 void setup() {
    // ... 생략 ...
    
    // 사용자가 설정 마법사 페이지를 통하여 입력한 값을 토대로 연결을 시도한다.
    // 만약 설정 값이 존재하지 않을 경우 바로 설정 모드로 진입한다.
    _ESP8266ConfigurationWizard.connect();

    // 설정모드 진입. setup() 외에 어디서든지 호출 가능하다.
    // _ESP8266ConfigurationWizard.startConfigurationMode();
  
}

void loop() {
  // 만약 연결이 끊어진경우 재접속을 시도한다. (최대한 딜레이가 발생하지 않도록 처리함)
  _ESP8266ConfigurationWizard.loop();
  if(_ESP8266ConfigurationWizard.isConfigurationMode()) return;
  
}
```
### 상태 이벤트 받기
  * setup() 함수에서 connect() 를 호출하기 전에 이벤트를 정의해야합니다. 
```cpp

void onStatusCallback(int status); 

void setup() {
    // ... 생략 ...
    _ESP8266ConfigurationWizard.setOnStatusCallback(onStatusCallback);
    _ESP8266ConfigurationWizard.connect();
    // ... 생략 ...
}
    
void onStatusCallback(int status) {
  // 설정 모드 진입. 
  if(status == STATUS_CONFIGURATION) {
      Serial.println("\n\nStart configuration mode");
  }
  // Wifi 연결 시도(혹은 재시도)
  else if(status == WIFI_CONNECT_TRY) {
    Serial.println("Try to connect to Wifi.");
  }
  // WiFi 연결 에러
  else if(status == WIFI_ERROR) {
    Serial.println("WIFI connection error.");
  }
  // WiFi 연결 성공
  else if(status == WIFI_CONNECTED) {
    Serial.println("WIFI connected.");
  }
  // NTP 서버 연결 시도(혹은 재시도)
  else if(status == NTP_CONNECT_TRY) {
    Serial.println("Try to connect to NTP Server.");
  }
  // NTP 서버 접속 에러 
  else if(status == NTP_ERROR) {
    Serial.println("NTP Server connection error.");
  }
  // NTP 연결 성공
  else if(status == NTP_CONNECTED) {
    Serial.println("NTP Server connected.");
  }
  // MQTT 연결 시도(혹은 재시도)
  else if(status == MQTT_CONNECT_TRY) {
    Serial.println("Try to connect to MQTT Server.");
  }
  // MQTT 연결 애러
  else if(status == MQTT_ERROR) {
    Serial.println("MQTT Server connection error.");
  }
  // MQTT 연결 성공
  else if(status == MQTT_CONNECTED) {
    Serial.println("MQTT Server connected.");
  }
  // 모든 연결 성공
  else if(status == STATUS_OK) {
    Serial.println("All connections are fine.");

    int day = _ESP8266ConfigurationWizard.getDay();
   int hour = _ESP8266ConfigurationWizard.getHours();
   int min = _ESP8266ConfigurationWizard.getMinutes();
   int sec = _ESP8266ConfigurationWizard.getSeconds();

    // 이 곳에서 mqtt 연결 초기화 하는 것을 권장합니다. 
    //_mqttClinet->setCallback(callbackSubscribe);
    //_mqttClinet->subscribe("topic");
}    


```

  
## 이 모듈을 사용하는 프로젝트
   * https://github.com/ice3x2/IOTForBlueAirPure

