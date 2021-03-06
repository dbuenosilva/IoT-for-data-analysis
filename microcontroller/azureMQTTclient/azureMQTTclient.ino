/*##########################################################################
# Project: PROG6002 - Programming Internet of Things - Assigment 2
# File: azureMQTTclient.ino
# Author: Diego Bueno - d.bueno.da.silva.10@student.scu.edu.au 
# Date: 18/09/2021
# Description: Arduino MQTT client to connect the device to Azure IoT Platform
#
# Credits:
#                   Adapted from Microsoft Azure SDK for C library - ESP8266 example
#                   https://github.com/Azure/azure-sdk-for-c/tree/main/sdk/samples/iot/aziot_esp8266
#                   Copyright (c) Microsoft Corporation. All rights reserved.
#                   SPDX-License-Identifier: MIT
#                   opencode@microsoft.com
#
# References:
#
# Jaycar manuals     https://www.jaycar.com.au/medias/sys_master/images/images/9566822498334/XC3802-manualMain.pdf
#                    https://www.jaycar.com.au/medias/sys_master/images/images/9566880923678/XC4411-manualMain.pdf
#                   
# DHT11 sensor lib   https://github.com/winlinvip/SimpleDHT 
#
# Azure IoT hub lib  https://www.arduino.cc/reference/en/libraries/azureiothub/
#                    https://github.com/Azure/azure-iot-arduino
# Azure MQTT client  https://github.com/Azure/azure-iot-arduino-protocol-mqtt
# Azure IoT Utility  https://github.com/Azure/azure-iot-arduino-utility
# Azure Iot Wi-Fi    https://github.com/Azure/azure-iot-arduino-socket-esp32-wifi
# Azure IoT cli      https://docs.microsoft.com/en-au/cli/azure/iot?view=azure-cli-latest
# Azure SDK for C    https://github.com/Azure/azure-sdk-for-c
#
# Arduino MQTT API   https://pubsubclient.knolleary.net/api
#
# Json lib           https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
#
##########################################################################>
# Changelog                            
# Author:                          
# Date:                                                        
# Description:     
#
##########################################################################>*/

#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <cstdlib>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <bearssl/bearssl.h>
#include <bearssl/bearssl_hmac.h>
#include <libb64/cdecode.h>

#include <az_result.h>
#include <az_span.h>
#include <az_iot_hub_client.h>

#include "iot_configs.h"
#include "ca.h"

// Status LED: will remain high on error and pulled high for a short time for each successful send.
#define LED_PIN 2
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
#define ONE_HOUR_IN_SECS 3600
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define MQTT_PACKET_SIZE 1024
#define TELEMETRY_FREQUENCY_MILLISECS 3000

static const char* ssid = IOT_CONFIG_WIFI_SSID;
static const char* password = IOT_CONFIG_WIFI_PASSWORD;
static const char* host = IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id = IOT_CONFIG_DEVICE_ID;
static const char* device_key = IOT_CONFIG_DEVICE_KEY;
static const int port = 8883;

static WiFiClientSecure wifi_client;
static X509List cert((const char*)ca_pem);
static PubSubClient mqtt_client(wifi_client);
static az_iot_hub_client client;
static char sas_token[200];
static uint8_t signature[512];
static unsigned char encrypted_signature[32];
static char base64_decoded_device_key[32];
static unsigned long next_telemetry_send_time_ms = 0;
static char telemetry_topic[128];
static uint8_t telemetry_payload[200];
static uint32_t telemetry_send_count = 0;

// Variables used to receive data from Arduino/Sensor
const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data
char typeOfData = ' ';
boolean newData = false;
int temperature = 0;
int humidity    = 0;

static void connectToWiFi()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to WIFI SSID ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

static void initializeTime()
{
  Serial.print("Setting time using SNTP");

  configTime(-5 * 3600, 0, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < 1510592825)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
}

static char* getCurrentLocalTimeString()
{
  time_t now = time(NULL);
  return ctime(&now);
}

static void printCurrentTime()
{
  Serial.print("Current time: ");
  Serial.print(getCurrentLocalTimeString());
}

void receivedCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
}

static void initializeClients()
{
  wifi_client.setTrustAnchors(&cert);
  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)host, strlen(host)),
          az_span_create((uint8_t*)device_id, strlen(device_id)),
          NULL)))
  {
    Serial.println("Failed initializing Azure IoT Hub client");
    return;
  }

  mqtt_client.setServer(host, port);
  mqtt_client.setCallback(receivedCallback);
}

static uint32_t getSecondsSinceEpoch()
{
  return (uint32_t)time(NULL);
}

static int generateSasToken(char* sas_token, size_t size)
{
  az_span signature_span = az_span_create((uint8_t*)signature, sizeofarray(signature));
  az_span out_signature_span;
  az_span encrypted_signature_span
      = az_span_create((uint8_t*)encrypted_signature, sizeofarray(encrypted_signature));

  uint32_t expiration = getSecondsSinceEpoch() + ONE_HOUR_IN_SECS;

  // Get signature
  if (az_result_failed(az_iot_hub_client_sas_get_signature(
          &client, expiration, signature_span, &out_signature_span)))
  {
    Serial.println("Failed getting SAS signature");
    return 1;
  }

  // Base64-decode device key
  int base64_decoded_device_key_length
      = base64_decode_chars(device_key, strlen(device_key), base64_decoded_device_key);

  if (base64_decoded_device_key_length == 0)
  {
    Serial.println("Failed base64 decoding device key");
    return 1;
  }

  // SHA-256 encrypt
  br_hmac_key_context kc;
  br_hmac_key_init(
      &kc, &br_sha256_vtable, base64_decoded_device_key, base64_decoded_device_key_length);

  br_hmac_context hmac_ctx;
  br_hmac_init(&hmac_ctx, &kc, 32);
  br_hmac_update(&hmac_ctx, az_span_ptr(out_signature_span), az_span_size(out_signature_span));
  br_hmac_out(&hmac_ctx, encrypted_signature);

  // Base64 encode encrypted signature
  String b64enc_hmacsha256_signature = base64::encode(encrypted_signature, br_hmac_size(&hmac_ctx));

  az_span b64enc_hmacsha256_signature_span = az_span_create(
      (uint8_t*)b64enc_hmacsha256_signature.c_str(), b64enc_hmacsha256_signature.length());

  // URl-encode base64 encoded encrypted signature
  if (az_result_failed(az_iot_hub_client_sas_get_password(
          &client,
          expiration,
          b64enc_hmacsha256_signature_span,
          AZ_SPAN_EMPTY,
          sas_token,
          size,
          NULL)))
  {
    Serial.println("Failed getting SAS token");
    return 1;
  }

  return 0;
}

static int connectToAzureIoTHub()
{
  size_t client_id_length;
  char mqtt_client_id[128];
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    Serial.println("Failed getting client id");
    return 1;
  }

  mqtt_client_id[client_id_length] = '\0';

  char mqtt_username[128];
  // Get the MQTT user name used to connect to IoT Hub
  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  {
    printf("Failed to get MQTT clientId, return code\n");
    return 1;
  }

  Serial.print("Client ID: ");
  Serial.println(mqtt_client_id);

  Serial.print("Username: ");
  Serial.println(mqtt_username);

  mqtt_client.setBufferSize(MQTT_PACKET_SIZE);

  while (!mqtt_client.connected())
  {
    time_t now = time(NULL);

    Serial.print("MQTT connecting ... ");

    if (mqtt_client.connect(mqtt_client_id, mqtt_username, sas_token))
    {
      Serial.println("connected.");
    }
    else
    {
      Serial.print("failed, status code =");
      Serial.print(mqtt_client.state());
      Serial.println(". Try again in 5 seconds.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);

  return 0;
}

void establishConnection() 
{
  connectToWiFi();
  initializeTime();
  printCurrentTime();
  initializeClients();

  // The SAS token is valid for 1 hour by default in this sample.
  // After one hour the sample must be restarted, or the client won't be able
  // to connect/stay connected to the Azure IoT Hub.
  if (generateSasToken(sas_token, sizeofarray(sas_token)) != 0)
  {
    Serial.println("Failed generating MQTT password");
  }
  else
  {
    connectToAzureIoTHub();
  }

  digitalWrite(LED_PIN, LOW);
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  establishConnection();
}

void getDataFromSensor() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

char* substr(char* arr, int begin, int len)
{
    char* res = new char[len + 1];
    for (int i = 0; i < len; i++)
        res[i] = *(arr + begin + i);
    res[len] = '\0';
    return res;
}

static char* getTelemetryPayload()
{
  char *eptr;

  getDataFromSensor();
  if (newData == true) {
    Serial.print("Received data from sensor");
    Serial.println(receivedChars);
    newData = false; 

    // Splitting the string into temperature and humidity values
    if (receivedChars[0] == 'C') { // Temperature in Celsius
      // Converting the second and tirdh chars in int
      temperature = strtol(substr(receivedChars,1,2), &eptr, 10); 
    }
    if (receivedChars[0] == 'H') { // Humidity
      humidity = strtol(substr(receivedChars,1,2), &eptr, 10);
    }

    if (temperature > 0 && humidity > 0 ) {

      az_span temp_span = az_span_create(telemetry_payload, sizeof(telemetry_payload));
      temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("{ \"deviceId\": \"" IOT_CONFIG_DEVICE_ID "\", \"temperature\": "));

      // testing all string received in serial port
      //temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("{ \"receivedChars\": "));
      //temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_BUFFER( receivedChars ));
      //temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR(" }"));
      //temp_span = az_span_copy_u8(temp_span, '\0');  

      (void)az_span_u32toa(temp_span, temperature, &temp_span);
      temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR(", \"humidity\": "));
      (void)az_span_u32toa(temp_span, humidity, &temp_span);
      temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR(", \"msgCount\": "));  
      (void)az_span_u32toa(temp_span, telemetry_send_count++, &temp_span);  
      temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR(" }"));
      temp_span = az_span_copy_u8(temp_span, '\0');
    }
    
  }
  return (char*)telemetry_payload;
}
static void sendTelemetry()
{
  digitalWrite(LED_PIN, HIGH);
  Serial.print(millis());
  Serial.print(" ESP8266 Sending telemetry . . . ");
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    Serial.println("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }

  mqtt_client.publish(telemetry_topic, getTelemetryPayload(), false);
  Serial.println("OK");
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void loop()
{
  if (millis() > next_telemetry_send_time_ms)
  {
    // Check if connected, reconnect if needed.
    if(!mqtt_client.connected())
    {
      establishConnection();
    }

    sendTelemetry();
    next_telemetry_send_time_ms = millis() + TELEMETRY_FREQUENCY_MILLISECS;
  }

  // MQTT loop must be called to process Device-to-Cloud and Cloud-to-Device.
  mqtt_client.loop();
  delay(500);
}
