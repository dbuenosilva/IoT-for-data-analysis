// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOT_CONFIGS_H
#define IOT_CONFIGS_H

/**
 * WiFi setup
 */
#define IOT_CONFIG_WIFI_SSID            "WiFi-EC44"
#define IOT_CONFIG_WIFI_PASSWORD        "78725522"

/**
 * IoT Hub Device Connection String setup
 * Find your Device Connection String by going to your Azure portal, creating (or navigating to) an IoT Hub, 
 * navigating to IoT Devices tab on the left, and creating (or selecting an existing) IoT Device. 
 * Then click on the named Device ID, and you will have able to copy the Primary or Secondary Device Connection String to this sample.
 */
#define DEVICE_CONNECTION_STRING    "HostName=IoT-FFM.azure-devices.net;DeviceId=Microcontroller-Brisbane-QLD;SharedAccessKey=SZTFj1fNtsneM6njIf2YdRHyZbNaxmSNixEM80/CmB0="
#define IOT_CONFIG_IOTHUB_FQDN      "IoT-FFM.azure-devices.net"
#define IOT_CONFIG_DEVICE_ID        "Microcontroller-Brisbane-QLD" 
#define IOT_CONFIG_DEVICE_KEY       "SZTFj1fNtsneM6njIf2YdRHyZbNaxmSNixEM80/CmB0="


// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_HTTP

#endif /* IOT_CONFIGS_H */
