#include "EspMQTTClient.h"
#include "DHTesp.h"
#include <ArduinoJson.h>

// OLED를 위한 include 세팅
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// NodeMCU의 GPIO Pin mapping 설정
#define D3 0

// Sensors and Actuators
#define SND_PIN A0
// #define SND_PIN D7
#define DHT_PIN D3

// controlling devices을 위한 시그널 설정
#define DHTTYPE DHT22

/*
  mqtt_topic : led, usbled의 컨트롤 값을 송수신하는 topic이다.
*/
const char *mqtt_topic = "iot/21900699/sound";

/*
 previousMillis, interval:
 전체 코드의 Non-Blocking programming을 위해 사용되는 변수. 
 sound_sum:
 정해진 시간동안 측정한 사운드 센서 변화량 값을 누적하는 변수. (이후 평균값에 이용)
 pub_interval:
 변화량 값을 누적하는 시간
*/
unsigned long previousMillis = 0;
const long interval = 200;
const long pub_interval = 5000;
unsigned long sound_sum = 0;

/*
  dht : 온습도 센서에서 값을 받아오기 위한 변수이다.
  sound, temperature, humidity : 측정한 조도값, 온도값, 소리측정값 위한 변수이다.
  pub_data : json 문자열로 변환시킨 온습도 혹은 조도 값을 담기 위한 변수이다.
*/
DHTesp dht;
unsigned int sound;
float temperature, humidity;
char pub_data[200];

const char *WifiSSID = "yrs";
const char *WifiPW = ".dms,chd.";
// const char *WifiSSID = "AndroidHotspot33_17_82";
// const char *WifiPW = ".dms,chd.";
// const char *WifiSSID = "SJ";
// const char *WifiPW = "tjdwns1111";
// const char *WifiSSID = "Lixux";
// const char *WifiPW = "77777777";

#define mqtt_broker "sweetdream.iptime.org"
// #define mqtt_broker "192.168.137.140"
// #define mqtt_broker "192.168.180.71"
#define mqtt_clientname "PenguinMCU"
#define mqtt_port 1883
#define MQTTUsername "iot"
#define MQTTPassword "csee1414"

/*
  client : mqtt broker에 연결하기 위한 기본적인 세팅이다.
*/

EspMQTTClient client(
  WifiSSID,
  WifiPW,
  mqtt_broker,  
  MQTTUsername,   
  MQTTPassword,   
  mqtt_clientname,   
  mqtt_port              
);

void setup()
{
  Serial.begin(115200);

  // DHT22 초기화
  dht.setup(DHT_PIN, DHTesp::DHT22); 

  // 소리센서 초기화
  pinMode (SND_PIN, INPUT);

  WiFi.begin(WifiSSID, WifiPW);
}

void onConnectionEstablished(){}

// nonblocking procedure
void loop()
{
  client.loop();
  // currentMillis: 현재 millis() 값을 저장하는 변수
  unsigned long currentMillis = millis();
  float peakToPeak = 0;              // peak-to-peak level

  unsigned int signalMax = 0;        // minimum value
  unsigned int signalMin = 1024;     // maximum value

  while (millis() - currentMillis < interval)
  {
    sound = analogRead(SND_PIN);                     // get reading from microphone
    
    if (sound < 1024)                                // toss out spurious readings
    {
      if (sound > signalMax)
      {
        signalMax = sound;                           // save just the max levels
      }
      else if (sound < signalMin)
      {
        signalMin = sound;                           // save just the min levels
      }
    }
  }

  peakToPeak = signalMax - signalMin;                    // max - min = peak-peak amplitude
  int db = map(peakToPeak,0,1024,0,100);                 //calibrate for deciBels
  sound_sum += db;

  // Serial.println(peakToPeak);
  // Serial.println(db);

  if (currentMillis - previousMillis >= pub_interval) {
    previousMillis = currentMillis;

    int avg_sound = (int)(sound_sum / 25);

    // 온습도 센서에서 온도, 습도 값을 가져온다.
    temperature = dht.getTemperature();
    humidity = dht.getHumidity();

    // 아두이노에서 센서 값을 잘 가지고 오는지 확인하기 위한 Serial.print 코드
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("    Humidity: ");
    Serial.print(humidity);
    Serial.print("    sound: ");
    Serial.println(avg_sound);

    // sensor_data라는 json 객체를 생성 후, pub_data에 직렬화된 json 문자열을 넣는다.
    StaticJsonDocument<256> sensor_data;
    sensor_data["Temperature"] = round(temperature*10)/10;
    sensor_data["Humidity"] = round(humidity*10)/10;
    sensor_data["Sound"] = avg_sound;
    serializeJson(sensor_data, pub_data);

    // json 문자열을 mqtt_topic라는 topic으로 publish한다.
    if (client.publish(mqtt_topic, pub_data)) {
      // Serial.println("Publish successful");
    } else {
      // Serial.println("Publish failed");
    }

    sound_sum = 0;
  }
}


