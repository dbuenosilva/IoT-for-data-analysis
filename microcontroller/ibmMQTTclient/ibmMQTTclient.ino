/*
##########################################################################
# Project: PROG6002 - Programming Internet of Things - Assigment 2
# File: ibmMQTTclient.ino
# Author: Diego Bueno - d.bueno.da.silva.10@student.scu.edu.au 
# Date: 18/09/2021
# Description: Arduino MQTT client to connect the device to IBM IoT Platform
#              and send data read from sensors.
#
# Credits:
# MQTT lib: https://pubsubclient.knolleary.net/
# Json lib: https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
# Jaycar manuals: https://www.jaycar.com.au/medias/sys_master/images/images/9566822498334/XC3802-manualMain.pdf
##########################################################################>
# Changelog                            
# Author:                          
# Date:                                                        
# Description:     
#
##########################################################################>*/

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define ORG "08dpd7"
#define DEVICE_TYPE "Microcontroller"
#define DEVICE_ID "48-3F-DA-0C-4B-AE"
#define TOKEN "vCVJ_tZH57MGUhm0lV"

#ifndef STASSID
  #define STASSID "WiFi-EC44"
  #define STAPSK  "78725522"
#endif

//Wi-Fi connection variables
const char* ssid     = STASSID;
const char* password = STAPSK;

//MQTT connection variables
const char publishTopic[] = "iot-2/evt/status/fmt/json";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";

// IBM IoT connection variables
char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

// WiFiClient class to create TCP connections
WiFiClientSecure wifiClient; 

// The MQTT Client
//PubSubClient MQTTClient(server, 1883, callback, wifiClient);
PubSubClient MQTTClient; 

void callback(char* publishTopic, char* payload, unsigned int length) {
  Serial.println("callback invoked");
}

// Initialising the application
void setup() {
  Serial.begin(115200);
  initWiFi();

  void callback(char* publishTopic, char* payload, unsigned int payloadLength);
 
  MQTTClient.setClient(wifiClient);
  MQTTClient.setServer(server,1883);
  //PubSubClient client(wifiClient);
  
  int publishInterval = 30000; // 30 seconds
  long lastPublishMillis;  

}

// Execution
void loop() {

  // Check if Wi-Fi is off-line each minutes and try to reconnect it if fails
  if (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
    initWiFi(); 
  }
  else {    
    Serial.println("");
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Serial Number: ");
    getSerialNumber();

  publishToIBM();
  }

  // Will check again in 60s
  delay(60000);
  
}



void initWiFi() {
 
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}

// READS ARDUINO SERIAL FROM EEPROM
//
// reads the Serial of the Arduino from the 
// first 6 bytes of the EEPROM

char* getSerialNumber()
{
  char sID[7];
  for (int i=0; i<6; i++) {
    sID[i] = EEPROM.read(i);
  }
  Serial.println(sID);
  return(sID);
}

int counter = 0;
void publishToIBM() {

  // if (millis() – lastPublishMillis > publishInterval) {
  String payload = "{\"d\":";
  payload += counter++;
  payload += "}";
  /*const size_t bufferSize = 2*JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  const char* payload = "a";*/

  Serial.print("Sending payload: ");
  Serial.println(payload);
  // client.publish(publishTopic, payload);
  //if(client.connected())
  //{
  // client.publish(publishTopic, (char *)payload.c_str());
  if (MQTTClient.publish(publishTopic, (char *)payload.c_str())) {
    Serial.println("Publish ok");
    if (!!!MQTTClient.connected()) {
      Serial.print("Reconnecting client to ");
      Serial.println(server);
      if (!!!MQTTClient.connect(clientId, authMethod, token)) {
          Serial.print(".");
          delay(500);
      }
      Serial.println();
    }
  } 
  else {
    Serial.println("Publish failed");
    if (!!!MQTTClient.connected()) {
      Serial.print("Reconnecting client to ");
      Serial.println(server);
      while (!!!MQTTClient.connect(clientId, authMethod, token)) {
        Serial.print(".");
        delay(500);
      }
      Serial.println();
    }
  }
}
