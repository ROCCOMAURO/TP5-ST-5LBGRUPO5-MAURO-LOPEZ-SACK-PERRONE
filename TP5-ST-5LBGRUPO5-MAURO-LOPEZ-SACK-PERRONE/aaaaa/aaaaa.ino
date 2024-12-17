//Grupo 4 Matias Cambiasso, Gabriel de Los Santos, Gabriel Mendelovich
#include <ArduinoJson.h>


#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"


FirebaseData fbdo;
FirebaseConfig config;
FirebaseAuth auth;


String uid;


#define USER_EMAIL "firebase@gmail.com"
#define USER_PASSWORD "pepito"


#include <WiFi.h>
const char* ssid = "AndroidAP";
const char* password = "douu7733";


#include <Wire.h>
#include <DHT.h>
#include <DHT_U.h>
#include <U8g2lib.h>


enum estado {
  PANTALLA1,
  SOLTARBTNS,
  PANTALLA2,
  REGRESO1,
  ESPERARSOLTARBTN1,
  ESPERARSOLTARBTN2
};


estado estadoActual = PANTALLA1;


#define DHTPIN 23
#define DHTTYPE DHT11
#define LED 25
#define BTN1 34
#define BTN2 35


DHT dht(DHTPIN, DHTTYPE);


U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


FirebaseJson json;


String databasePath;
String tempPath = "/temperature";
String timePath = "/timeStamp";
String parentPath;


int caseCicle = 30;
float temperature = 24;
unsigned long sendDataPrevMillis = 0;
unsigned long timestamp;


unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}


//----------------------------------------------------//


void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  Serial.begin(9600);
  u8g2.begin();


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to wifi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");


  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;


  config.api_key = "AIzaSyDd4HnyRe2MLBukPGUTFu0JMYNsQsBu1YQ";
  config.database_url = "https://tp5mirko-default-rtdb.firebaseio.com/";
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);


  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;


  Firebase.begin(&config, &auth);


  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }


  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);


  databasePath = "/UsersData/" + uid + "/readings";  // (carpetas rey)


  dht.begin();
  timeClient.begin();
}


 //----------------------------------------------------//


  void loop() {
    temperature = dht.readTemperature();


    maquina();


    unsigned long milliCicle = caseCicle * 1000;


    if (Firebase.ready() && ((millis() - sendDataPrevMillis) > milliCicle || sendDataPrevMillis == 0)) {
      sendDataPrevMillis = millis();
      timestamp = getTime();
      Serial.print("time: ");
      Serial.println(timestamp);


      parentPath = databasePath + "/" + String(timestamp);


      json.set(tempPath.c_str(), String(temperature));  //asignas aca clave-valor pa
      json.set(timePath.c_str(), String(timestamp));
      Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    }
  }




void maquina() {
  switch (estadoActual) {


   case PANTALLA1:
     
      printBMP_OLED(temperature, caseCicle);


      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
        estadoActual = SOLTARBTNS;
      }
      break;


    case SOLTARBTNS:
      if (digitalRead(BTN1) == HIGH && digitalRead(BTN2) == HIGH) {
        estadoActual = PANTALLA2;
      }
      break;


    case REGRESO1:
      if (digitalRead(BTN1) == HIGH && digitalRead(BTN2) == HIGH) {
        sendDataPrevMillis = 0;
        estadoActual = PANTALLA1;
      }
      break;


    case PANTALLA2:
   
    S2OLED(caseCicle);


      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == HIGH) {
        estadoActual = ESPERARSOLTARBTN1;
      }
      if (digitalRead(BTN1) == HIGH && digitalRead(BTN2) == LOW) {
        estadoActual = ESPERARSOLTARBTN2;
      }
      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
        estadoActual = REGRESO1;
      }
      break;


    case ESPERARSOLTARBTN1:
      if (digitalRead(BTN1) == HIGH) {
        if (caseCicle > 30) {
          caseCicle = caseCicle - 30;
        }
        estadoActual = PANTALLA2;
      }
      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
        estadoActual = REGRESO1;
      }
      break;


    case ESPERARSOLTARBTN2:
      if (digitalRead(BTN2) == HIGH) {
        caseCicle = caseCicle + 30;
        estadoActual = PANTALLA2;
      }
      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
        estadoActual = REGRESO1;
      }
      break;
  }
}


void S2OLED(int saveCicle) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.setCursor(32, 30);
  u8g2.print(saveCicle);
  u8g2.drawStr(80, 30, "s");
  u8g2.drawStr(15, 50, "-");
  u8g2.drawStr(100, 50, "+");

  u8g2.sendBuffer();
}



void printBMP_OLED(float temp, int saveCicle) {
  u8g2.clearBuffer();  // clear the internal memory

  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(32, 15, "TP 5");

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 30, "Temperatura: ");
  u8g2.setCursor(80, 30);
  u8g2.print(temp);
  u8g2.drawStr(110, 30, "°C");
  u8g2.drawStr(0, 45, "Ciclo:");
  u8g2.setCursor(80, 45);
  u8g2.print(saveCicle);
  u8g2.drawStr(110, 45, "s");


  u8g2.sendBuffer();  // transfer internal memory to the display
}
