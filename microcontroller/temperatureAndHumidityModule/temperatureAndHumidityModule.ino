/*
##########################################################################
# Project: PROG6002 - Programming Internet of Things - Assigment 2
# File: temperatureAndHumidityModule.ino
# Author: Diego Bueno - d.bueno.da.silva.10@student.scu.edu.au 
# Date: 21/09/2021
# Description: Temperature and Humidity module to read data from DHT11
#              sensor and write on serial port like:
#
#              C99
#              H99
#              EXX
#    Where:
#              C99 is temperature in Celsius
#              H99 represents the humidity.
#              EXX is the message error
#
# Reference:
#
# DHT11 sensor lib : https://github.com/winlinvip/SimpleDHT 
#
##########################################################################>
# Changelog                            
# Author:                          
# Date:                                                        
# Description:     
#
##########################################################################>*/

#include <SimpleDHT.h>

// DHT11 works at rate of 1HZ, so define interval of 1,5 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 1500

// Limit of temperature for Queensland to enable devices
#define INCREASE_TEMPERATURE_LIMIT 1
#define INCREASE_HUMIDIT_LIMIT     5

// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 11 PIN
int pinDHT11 = 11;

// for turning ON/OFF devices according to temperature limit, 
//      LOW: 0V device is ON
//      HIGHT: 5V device is OFF 
//      INSTRUCTION: 3 PIN
int pinRelay = 3;

// The initial temperature and humidity
int initialTemperature = 0;
int initialHumidity    = 0;

// Creating an intance of SimpleDHT11 class
SimpleDHT11 dht11(pinDHT11);

void setup() {
  Serial.begin(115200);

  // Define Relay out PIN
  pinMode(pinRelay, OUTPUT);

  // Initialise with devices off
  digitalWrite(pinRelay, LOW);
}

void loop() {
  
  // read without samples.
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    /* If fails, send to serial port the standard:
      EXXXXXXXX
      
      Where X is the message error.
    */
    Serial.print("E: Reading failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); 
    Serial.print(SimpleDHTErrDuration(err)); 
    delay(1000);
    Serial.print('\n');
  }
  else {
    /* If sucess, send to serial port the standard:
     *  
      C99
      H99
      
      Where C99 is temperature in Celsius and H99 represents the humidity.
    */
    Serial.print("C"); Serial.print((int)temperature); Serial.print('\n');
    Serial.print("H"); Serial.print((int)humidity); Serial.print('\n');

    if (initialTemperature == 0 ) {
      initialTemperature = (int)temperature;
    }

    if (initialHumidity == 0 ) {
      initialHumidity = (int)humidity;
    }

    // If temperature or humidity increase over the limit, turn on devices
    if ( ( initialTemperature > 0 && (int)temperature >= ( initialTemperature + INCREASE_TEMPERATURE_LIMIT ))
        || ( initialHumidity > 0 && (int)humidity >= ( initialHumidity + INCREASE_HUMIDIT_LIMIT )) ) {
        digitalWrite(pinRelay, HIGH);
    }
    else { // turn OFF device
        digitalWrite(pinRelay, LOW);
    }
  }
  
  // DHT11 works at rate of 1HZ. 
  delay(TELEMETRY_FREQUENCY_MILLISECS);
}
