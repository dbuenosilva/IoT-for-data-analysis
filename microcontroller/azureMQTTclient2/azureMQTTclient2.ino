/*
##########################################################################
# Project: PROG6002 - Programming Internet of Things - Assigment 2
# File: azureMQTTclient.ino
# Author: Diego Bueno - d.bueno.da.silva.10@student.scu.edu.au 
# Date: 18/09/2021
# Description: Arduino MQTT client to connect arduino device to Azure IoT Platform
#             
#
# Credits:
#
# Adapted from Andri Yadi example on AzureIoTHubMQTTClient Arduino library.
#                    https://github.com/adafruit/Adafruit-BMP085-Library
#
# References:
#
# Jaycar manuals   : https://www.jaycar.com.au/medias/sys_master/images/images/9566822498334/XC3802-manualMain.pdf
#
# Azure IoT hub lib: https://www.arduino.cc/reference/en/libraries/azureiothub/
#                    https://github.com/Azure/azure-iot-arduino
# Azure MQTT client: https://github.com/andriyadi/AzureIoTHubMQTTClient
# Azure IoT Utility: https://github.com/Azure/azure-iot-arduino-utility
# Azure Iot Wi-Fi  : https://github.com/Azure/azure-iot-arduino-socket-esp32-wifi
# Json lib         : https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
# NtpClientLib     : https://github.com/gmag11/NtpClient 
# Adafruit BPM085  : https://github.com/adafruit/Adafruit-BMP085-Library 
#
##########################################################################>
# Changelog                            
# Author:                          
# Date:                                                        
# Description:     
#
##########################################################################>*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <AzureIoTHubMQTTClient.h>

const char *AP_SSID = "WiFi-EC44";
const char *AP_PASS = "78725522";

// Serial port settings
#define SERIAL_PORT_SPEED       115200 // rate transfer in bps
#define SERIAL_PORT_DELAY       2000 // in milisecondes

// Azure IoT Hub Settings
#define IOTHUB_HOSTNAME         "IoT-FFM.azure-devices.net"
#define DEVICE_ID               "Microcontroller-Brisbane-QLD"
#define DEVICE_KEY              "SZTFj1fNtsneM6njIf2YdRHyZbNaxmSNixEM80/CmB0="
#define PUBLISH_INTERVAL        3000

#define USE_BMP180              1 //Set this to 0 if you don't have the sensor and generate random sensor value to publish

// Creating Wi-Fi client with TLS secure
WiFiClientSecure tlsClient;

// Creating client AzureIoTHubMQTTClient object
AzureIoTHubMQTTClient client(tlsClient, IOTHUB_HOSTNAME, DEVICE_ID, DEVICE_KEY);

// Creating handle WiFi to monitor events
WiFiEventHandler  e1, e2;

#if USE_BMP180
    #include <Adafruit_BMP085.h>
    Adafruit_BMP085 bmp;
#endif

const int LED_PIN = 15; //Pin to turn on/of LED a command from IoT Hub
unsigned long lastMillis = 0;

void connectToIoTHub(); // <- predefine connectToIoTHub() for setup()
void onMessageCallback(const MQTT::Publish& msg);

void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
    Serial.printf("Wi-Fi interface connected! IP Address: %s\r\n", ipInfo.ip.toString().c_str());

    //do connect upon WiFi connected
    connectToIoTHub();
}

void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
    Serial.printf("Wi-Fi interface disconnected from SSID %s\n", event_info.ssid.c_str());
    Serial.printf("Reason: %d\n", event_info.reason);
}

void onClientEvent(const AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEvent event) {
    if (event == AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEventConnected) {

        Serial.println("Connected to Azure IoT Hub!");

        //Add the callback to process cloud-to-device message/command
        client.onMessage(onMessageCallback);
    }
}

void onActivateRelayCommand(String cmdName, JsonVariant jsonValue) {

    //Parse cloud-to-device message JSON. In this example, I send the command message with following format:
    //{"Name":"ActivateRelay","Parameters":{"Activated":0}}

    JsonObject& jsonObject = jsonValue.as<JsonObject>();
    if (jsonObject.containsKey("Parameters")) {
        auto params = jsonValue["Parameters"];
        auto isAct = (params["Activated"]);
        if (isAct) {
            Serial.println("Activated true");
            digitalWrite(LED_PIN, HIGH); //visualize relay activation with the LED
        }
        else {
            Serial.println("Activated false");
            digitalWrite(LED_PIN, LOW);
        }
    }
}

/*
    setup() runs at initialisation of the microcontroller, all process start here
*/
void setup() {

    Serial.begin(SERIAL_PORT_SPEED);

    while(!Serial) {
        yield();
    }
    delay(SERIAL_PORT_DELAY);

    Serial.setDebugOutput(true);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    #if USE_BMP180
        if (bmp.begin()) {
            Serial.println("BMP INIT SUCCESS");
        }
    #endif

    Serial.print("Connecting to WiFi...");
    //Begin WiFi joining with provided Access Point name and password
    WiFi.begin(AP_SSID, AP_PASS);

    //Handle WiFi events
    e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start the Client invoking the onSTAGotIP method
    e2 = WiFi.onStationModeDisconnected(onSTADisconnected); // If WiFi is disconnected, invoke the onSTADisconnected method

    //Handle client events.
    // client is a instance of AzureIoTHubMQTTClient class
    client.onEvent(onClientEvent);

    //Add command to handle and its handler. The onActivateRelayCommand method is executed 
    //Command format is assumed like this: {"Name":"[COMMAND_NAME]","Parameters":[PARAMETERS_JSON_ARRAY]}
    client.onCloudCommand("ActivateRelay", onActivateRelayCommand);
}

void onMessageCallback(const MQTT::Publish& msg) {

    //Handle Cloud to Device message by yourself.

//    if (msg.payload_len() == 0) {
//        return;
//    }
    Serial.println("Callback executed!");
    Serial.println(msg.payload_string());
}

void connectToIoTHub() {

    Serial.print("\nConnecting to Azure IoT Hub Client... ");
    if (client.begin()) {
        Serial.println("Connected!");
    } else {
        Serial.println("Could not connect to MQTT Broker");
    }
}

void readSensor(float *temp, float *press) {

#if USE_BMP180
    *temp = bmp.readTemperature();
    *press = 1.0f*bmp.readPressure()/1000; //--> kilo
#else
    //If you don't have the sensor
    *temp = 20 + (rand() % 10 + 2);
    *press = 90 + (rand() % 8 + 2);
#endif

}

/*
    loop() runs as an infinit loop while the microcontroller is on
*/
void loop() {

    client.run(); //MUST CALL THIS in loop()

    if (client.connected()) {

        // Publish a message roughly every (PUBLISH_INTERVAL/1000) seconds. Only after time is retrieved and set properly.
        if(millis() - lastMillis > PUBLISH_INTERVAL && timeStatus() != timeNotSet) {
            lastMillis = millis();

            //Read the actual temperature from sensor
            float temp, press;
            readSensor(&temp, &press);

            //Get current timestamp, using Time lib
            time_t currentTime = now();

            // You can do this to publish payload to IoT Hub
            /*
            String payload = "{\"DeviceId\":\"" + String(DEVICE_ID) + "\", \"MTemperature\":" + String(temp) + ", \"EventTime\":" + String(currentTime) + "}";
            Serial.println(payload);

            //client.publish(MQTT::Publish("devices/" + String(DEVICE_ID) + "/messages/events/", payload).set_qos(1));
            client.sendEvent(payload);
            */

            //Or instead, use this more convenient way
            AzureIoTHubMQTTClient::KeyValueMap keyVal = {{"MTemperature", temp}, {"MPressure", press}, {"DeviceId", DEVICE_ID}, {"EventTime", currentTime}};
            client.sendEventWithKeyVal(keyVal);
        }
    }
    else {

    }

    delay(10); // <- fixes some issues with WiFi stability. Probably, the WAP deny the connection for overloading the WiFi network
}
