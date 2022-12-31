/**
2022년 12월, 웹 대시보드 앱과 연동을 위한 아두이노 코드
사용언어 : C++, Javascript
사용 플랫폼 : MQTT
제작 : 문지원
*/

/* 스마트팜에서 입출력 센서를 사용하기 위한 라이브러리를 가져옵니다. */
#include "DHT.h"
#include <Wire.h>
#include <Servo.h>
#include "LiquidCrystal_I2C.h"
// #include "RTClib.h"
#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspUdp.h>
#include <PubSubClient.h> // MQTT 서버 통신을 위한 라이브러리

/* 스마트팜의 각 센서 핀을 정의 */
#define DHTPIN 12             // 온습도 핀
#define DHTTYPE DHT11         // 보드에 장착된 온습도 센서의 모델명
#define SERVOPIN 9            // 창문(서보모터) 핀
#define LIGHTPIN 4         // 천장 조명 핀
#define FAN_PIN 32            // 팬 핀
#define WATER_PUMP_PIN 31     // 펌프 핀
#define CDC_PIN 0
#define SEND_DELAY 500       // 센서값 전송 간격(단위 : ms)
#define WINDOW_OPEN_ANGLE 80  // 창문이 열렸을때의 서보모터 각도 (서보모터 설치 위치에 따라 다르기 때문에 추후 수정)
#define WINDOW_CLOSE_ANGLE 0  // 창문이 열렸을때의 서보모터 각도 (서보모터 설치 위치에 따라 다르기 때문에 추후 수정)

IPAddress server(0, 0, 0, 0); // MQTT 주소

DHT dht(DHTPIN, DHTTYPE);
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
// RTC_DS3231 rtc;

WiFiEspClient espClient;
PubSubClient client(espClient);

const char ssid[] = "network SSID"; // 네트워크 이름
const char pass[] = "network Password"; // 네트워크 비밀번호

/*
아래의 작성 요령)
const char farmName[] = "Firebase의 스마트팜 문서 아이디";
const char mqttId[] = "Firebase의 스마트팜 mqttId"
const char mqttId[] = "Firebase의 스마트팜 mqttId에서re만 붙이도록"

예시)
const char farmName[] = "SmartFarm1";
const char mqttId[] = "Farm1"
const char mqttId[] = "Farm1re"
*/
const char farmName[] = "";
const char mqttId[] = "";
const char mqttIdRe[] = ""; // re부분 제외 mqttId와 동일하게 설정해주세요

bool isFan = false; // 팬이 돌아가고 있는지 여부를 판단합니다.
bool isWindow = false; // 창문이 열려있는지 확인합니다.
bool isPump = false; // 펌프 작동여부
// bool isRTC = false; // RTC 미사용

// 정수형 변수
int lightBright = 0; // LED 라이트의 밝기를 저장합니다
int cdc = 0; // 조도센서의 값을 저장합니다
int humid = 0; // 온습도 센서의 습도값을 저장합니다
int temp = 0; // 온습도 센서의 온도값을 저장합니다


// 조건변수의 변수
// 조도항목
bool isCdcAuto = false; // 조건변수 활성화 여부
bool isCdcAutoUp = false; // 조건변수 이상 및 이하 여부
int cdcAuto = 0; // 조건변수 조건
// 온도항목
bool isTempAuto = false;
bool isTempAutoUp = false;
int tempAuto = 0;
// 습도항목
bool isHumiAuto = false;
bool isHumiAutoUp = false;
int humiAuto = 0;


void setup() {
  // 시리얼 및 LCD 초기설정
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  dht.begin();
  
  Serial.println("[INFO] Starting...");
  printLcd("Check ESP8266");
  Serial.println("[INFO] 통신 모듈을 확인합니다");
  Serial2.begin(115200);
  WiFi.init(&Serial2);
  if(WiFi.status() == WL_NO_SHIELD){
    Serial.println("[ERROR] 와이파이 통신 모듈을 찾을 수 없습니다");
    printLcd("Not found ESP01");
    return;
  }
  Serial.println("[INFO] 와이파이와 연결을 시도합니다");
  printLcd("CNT to WiFi..");
  WiFi.begin(ssid,pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.println("[INFO] 네트워크와 연결되었습니다");
  printLcd("CNTED to WiFi");
  delay(500);
  //RTC 모듈 필요시 아래 주석 해제
//  Serial.println("[INFO] RTC 모듈 시작");
//  printLcd("Starting RTC..");
//  startRTC();

  // MQTT 서버 설정
  Serial.println("[INFO] MQTT 브로커와 통신합니다");
  printLcd("Setting to MQTT");
  client.setServer(server, 1883); // MQTT 서버를 설정하는 코드이며 MQTT 기본 포트는 1883입니다
  client.setCallback(messageCallback);
  servo.attach(SERVOPIN);
  
}

/*
 아두이노 필수 루프 함수로 MQTT와 연결이 끊겼을 경우 재연결을 시도하고 MQTT의 연결이 끊기지 않도록 유지해줍니다
*/
void loop() {
  if(!client.connected()){
    connect();
  }
  temp = dht.readTemperature();
  humid = dht.readHumidity();
  cdc = map(analogRead(CDC_PIN),0,1023,0,100);
  bool lightValue = lightBright != 0 ? true : false;
  String sendMessage = "";
  sendMessage = String("sensorStatus,")+temp+","+humid+","+cdc+","+lightValue+","+isFan+","+isWindow;
  char result[50];
  sendMessage.toCharArray(result,50);
  client.loop();
  client.publish(mqttId,result);
  conditionCheck();
  
  delay(500); // 딜레이가 없으면 ESP에서 와이파이가 끊김
  
}

/*
 MQTT에서 구독한 주제에서 메시지를 받으면 실행되는 콜백함수입니다
*/
void messageCallback(char* topic, byte* payload, unsigned int length) {
  String command = "";
  // 전달 받은 데이터가 버퍼이기 때문에, 하나씩 가져와서 정수형 변수에 저장
  for (int i=0;i<length;i++) {
    command += (char)payload[i];
  }
  // 위에서 저장한 정수형 변수의 값에 명령어가 들어있는지 체크하는 조건문
   if(command.indexOf("setAutoCDC") != -1){
    // setAutoCDC=UP-true,12 (전송양식)
    isCdcAuto = command.substring(command.indexOf("-")+1,command.indexOf(",")) == "true";
    isCdcAutoUp = command.indexOf("UP") != -1;
    cdcAuto = command.substring(command.indexOf(",")+1,command.length()).toInt();
  }else if(command.indexOf("setAutoTemp") != -1){
    isTempAuto = command.substring(command.indexOf("-")+1,command.indexOf(",")) == "true";
    isTempAutoUp = command.indexOf("UP") != -1;
    tempAuto = command.substring(command.indexOf(",")+1,command.length()).toInt();
  }else if(command.indexOf("setAutoHumid")!= -1){
    isHumiAuto = command.substring(command.indexOf("-")+1,command.indexOf(",")) == "true";
    isHumiAutoUp = command.indexOf("UP") != -1;
    humiAuto = command.substring(command.indexOf(",")+1,command.length()).toInt();
  }else if(command.indexOf("setLight") != -1){
    // setLight-1 (전송양식)
    controlLight(command.substring(command.indexOf("-")+1,command.length()).toInt());
  }else if(command.indexOf("setServo") != -1){
    controlServo(command.substring(command.indexOf("-")+1,command.length()).toInt());
  }else if(command.indexOf("setFan") != -1){
    controlFan(command.substring(command.indexOf("-")+1,command.length()).toInt());
  }
}

/*
 MQTT 서버와 연결을 시도하는 함수입니다
 연결 후 reconnected 메시지를 전송합니다
*/
void connect() {
  while (!client.connected()) {
    Serial.println("[INFO] MQTT와 연결을 시도합니다");
    printLcd("CNT to MQTT..");
    if (client.connect(farmName)) {
      Serial.println("[INFO] MQTT와 연결되었습니다");
      printLcd("CNTED to MQTT");
      client.publish(mqttId,"reconnected");
      client.subscribe(mqttIdRe);
    } else {
      printLcd("Failed rc ="+client.state());
      Serial.print("[ERROR] MQTT 연결 실패, 오류코드 : rc=");
      Serial.print(client.state());
      Serial.println("[ERROR] 5초 후 MQTT와 재연결을 시도합니다");
      delay(5000);
    }
  }
}

/*
 LCD에 글씨를 출력할 때 쉽게 사용하기 위한 함수입니다
*/
void printLcd(String message){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(message); 
}

/* 
  RTC 모듈 확인
  필요 시 주석 해제
*/

//void startRTC(){
//  Serial.println("[INFO] RTC 모듈을 준비합니다");
//  if(!rtc.begin()){
//     Serial.println("[ERROR] RTC 모듈을 찾지 못했습니다");
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//    isRTC = false;
//    return;
//  }
//   Serial.println("[INFO] RTC 모듈의 시간을 컴퓨터에 저장된 시간으로 설정합니다");
//  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
//  isRTC = true;
//  return;
//}

/*
 조명을 제어하는데 사용하는 함수입니다
*/
void controlLight(int value){
  lightBright = value;
  int val = map(value,0,100,0,1023); // 전달받을 값이 0 ~ 100이기 떄문에 맵핑을 통해서 0 ~ 1023 값으로 변환
  analogWrite(LIGHTPIN,val);
}
/*
 팬을 제어하는데 사용하는 함수입니다
*/
void controlFan(bool value){
  isFan = value;
  digitalWrite(FAN_PIN,value);
}
/*
 서보모터를 제어하는데 사용하는 함수입니다
*/
void controlServo(bool value){
  isWindow = value;
  servo.write(value ? WINDOW_OPEN_ANGLE : WINDOW_CLOSE_ANGLE); 
}
/*
 각 센서의 센서값을 확인하여 조건변수의 활성화 여부에 따라 자동으로 센서를 키고 끄는 함수입니다
  대시보드의 조건 
*/
void conditionCheck(){
  if(isCdcAuto){
    if(isCdcAutoUp && cdcAuto <= cdc){
      controlLight(1);
    }else if(!isCdcAutoUp && cdcAuto >= cdc){
      controlLight(1);
    }else controlLight(0);
  }

  if(isTempAuto){
    
    if(isTempAutoUp && tempAuto <= temp){
      controlServo(1);
    }else if(!isTempAutoUp && tempAuto >= temp){
      controlServo(1);
    }else controlServo(0);
  }
  
  if(isHumiAuto){
    if(isHumiAutoUp && humiAuto <= humid){
      controlFan(1);
    }else if(!isHumiAutoUp && humiAuto >= humid){
      controlFan(1);
    }else controlFan(0);
  }
}
