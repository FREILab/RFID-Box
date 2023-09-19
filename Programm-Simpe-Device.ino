#include "config.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include "MFRC522.h"

/* wiring the MFRC522 to ESP8266 (ESP-12)
  SDA(SS) = GPIO4   --> I2C
  MOSI    = GPIO13  --> SPI
  MISO    = GPIO12  --> SPI
  SCK     = GPIO14  --> SPI
  GND     = GND
  3.3V    = 3.3V
*/

#define RST_PIN  3  // RST-PIN für RC522 - RFID - SPI - Modul 
#define SS_PIN  4  // SDA-PIN für RC522 - RFID - SPI - Modul

#define LED_RED 16
#define LED_YELLOW 0
#define LED_GREEN 2

#define SWITCH_PIN 15

#define SIGNAL_PIN 5

const char *ssid = WIFI_SSID;
const char *pass = WIFI_PASSWORD;

MFRC522 mfrc522(SS_PIN, RST_PIN);

String logedInID = "0";

int loginUpdateCounter = 0;

void setup() {
  pinMode(SWITCH_PIN, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT); 
  pinMode(SIGNAL_PIN, OUTPUT);

  digitalWrite(SIGNAL_PIN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, LOW);
  
  digitalWrite(LED_YELLOW, HIGH);
  
  Serial.begin(115200);
  
  delay(250);
  Serial.println(F("Booting...."));

  SPI.begin();
  mfrc522.PCD_Init();

  WiFi.begin(ssid, pass);

  int retries = 0;
  while ((WiFi.status() != WL_CONNECTED) && (retries < 10)) {
    retries++;
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi connected"));
    digitalWrite(LED_YELLOW, LOW);
  } else {
    digitalWrite(LED_RED, HIGH);
    while (true);
  }

  Serial.println(F("Ready!"));
  Serial.println(F("======================================================"));
  Serial.println(F("Scan for Card and print UID:"));
  digitalWrite(LED_GREEN, LOW);
}

void logout() {
  Serial.println("Loged out.");
  logedInID = "0";
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(SIGNAL_PIN, LOW);
}

void loop() {
  while(digitalRead(SWITCH_PIN) == 0){
    delay(100);
  }
  delay(250);
  String uid = readID();

  if(uid != "0") {
    tryLoginID(uid);
    delay(1000);
    while(digitalRead(SWITCH_PIN) == 1){
      updateLogin();
      delay(1000);
    }
    logout();
  } else {
    delay(250);
  }
}

String readID(){
  // Look for new cards
  for(int i = 0; i < 3; i++) {
    if(mfrc522.PICC_IsNewCardPresent()){
      if (mfrc522.PICC_ReadCardSerial()) {
        String uid = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          if (i != 0) {
            uid = uid + ":";
          }
          int b = mfrc522.uid.uidByte[i];
          if (b >= 16) {
            uid = uid + String(b, 16);
          } else {
            uid = uid + "0" + String(b, 16);
          }          
        }
        return uid;
      }      
    }
    delay(100);
  }
  return "0";
}

void tryLoginID(String uid) {
  digitalWrite(LED_YELLOW, HIGH);
  digitalWrite(LED_RED, LOW);
  HTTPClient http;
  WiFiClient client;

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  String s = "";
  s = s + "http://" + SERVER_IP + "/machine_try_login/" + AUTHENTICATION_TOKEN + "/" + MACHINE_NAME + "/" + uid;
  http.begin(client, s); //HTTP

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      if (payload.indexOf("true") >= 0) {
        logedInID = uid;
        Serial.println("Loged in: " + uid);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(SIGNAL_PIN, HIGH);
      } else {
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_RED, HIGH);
        delay(2000);
        digitalWrite(LED_RED, LOW);
      }
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    digitalWrite(LED_RED, HIGH);
  }
  http.end();
} 

void updateLogin() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  loginUpdateCounter++;
  if (loginUpdateCounter < 200) {
    return;
  }
  loginUpdateCounter = 0;
  HTTPClient http;
  WiFiClient client;					

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  String s = "";
  s = s + "http://" + SERVER_IP + "/machine_extend_login/" + AUTHENTICATION_TOKEN + "/" + MACHINE_NAME;
  http.begin(client, s); //HTTP

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, HIGH);
  }
  http.end();
}
