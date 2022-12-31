# 아두이노 사물인터넷 스마트팜
<img src="https://img.shields.io/badge/arduino-00979D?style=flat-square&logo=arduino&logoColor=white"/>

아두이노에서 MQTT 프로토콜을 통해서 웹에서 데이터를 주고 받아서 농장을 관리하는 프로젝트입니다.

### 주요 기능 
- 센서의 특정 조건에 맞추어 작동
- 와이파이 라이브러리를 통한 웹 대시보드와 통신
- 시간에 맞추어 작동하는 센서
- 대시보드의 설정값을 데이터베이스에 저장
- 다중 농장 관리 시스템
- MQTT 및 와이파이 연결 오류 시 재연결

### 사용 안내
- MQTT 브로커 및 프론트엔드, 백엔드 파일은 비공개
- 서버 주소는 작품 옆 수정 가이드를 참고
- 아래의 필수 라이브러리를 설치해야 사용이 가능

## 필수 라이브러리
- [Adafruit_Sensor (DHT 라이브러리를 사용하기 위함)](https://github.com/adafruit/Adafruit_Sensor)
- [DHT-sensor-library (온습도 센서 라이브러리)](https://github.com/adafruit/DHT-sensor-library)
- [LiquidCrystal_I2C (I2C 통신으로 연결된 LCD 사용을 위한 라이브러리)](https://github.com/johnrickman/LiquidCrystal_I2C)
- [WiFiEsp (와이파이 연결과 클라언트 사용을 위한 라이브러리)](https://github.com/bportaluri/WiFiEsp)
- [PubsubClient (와이파이 라이브러리의 클라이언트에서 MQTT 통신을 위한 라이브러리)](https://github.com/knolleary/pubsubclient)
- [Servo (서보모터 사용을 위한 라이브러리](https://github.com/arduino-libraries/Servo)

## 프로젝트 문의

해당 프로젝트 관련 문의는 moonjiwon20@naver.com으로 연락바랍니다
