# IoT-for-data-analysis
Providing data to Power BI from IoT sensors




# Useful Azure IoT cli commands:

Listing all IoT hubs: az iot hub list	
    see in the Json    "name": "IoT-FFM"

Listing specific hub: az iot hub show --name IoT-FFM

Monitoring the telemetry messages sent to the Azure IoT Hub by device: az iot hub monitor-events -n "IoT-FFM" -d "Microcontroller-Brisbane-QLD"

{
    "event": {
        "origin": "Microcontroller-Brisbane-QLD",
        "module": "",
        "interface": "",
        "component": "",
        "payload": "{ \"deviceId\": \"Microcontroller-Brisbane-QLD\", \"msgCount\": 494 }"
    }
}

Simulate a device in an Azure IoT Hub: az iot device simulate