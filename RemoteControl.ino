/*
 * Read from temp/humidity and current/voltage sensors and send data to
 * receiver module over serial connection).
 *
 *
 * Copyright (C) 2014 Mark A. Heckler (@MkHeck, mark.heckler@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Please use freely with attribution. Thank you!
 */

#include <dht11.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

/*
 * Hardware configuration
 */

// Set up sensors
dht11 DHT11;
Adafruit_INA219 ina219;

// And now the variables for capturing sensor values
int chk;
float busVoltage;
float shuntVoltage;
float current_mA;
float loadVoltage;
String msg;

// State & other variables
int inByte;
int lightPin = 5;         // LED pin
int powerPin = 12;        // Assign "for real" when relay is connected
boolean isAutonomous = true;
boolean isLightOn = false;
boolean isPowerOn = false;
int powerOnSeconds = 0;

void setup(void)
{
  Serial.begin(9600);

  Serial.println("Initializing...");
  
  /*
   * Initialize pin(s)
   */
   pinMode(lightPin, OUTPUT);
   pinMode(powerPin, OUTPUT);
  
  /*
   * Initialize temp/humidity sensor
   */
  DHT11.attach(2);
  ina219.begin();
}

void loop(void)
{
  // Get the temp/humidity
  chk = DHT11.read();

  // Get the current/voltage readings
  busVoltage = ina219.getBusVoltage_V();
  shuntVoltage = ina219.getShuntVoltage_mV();
  current_mA = ina219.getCurrent_mA();
  loadVoltage = busVoltage + (shuntVoltage / 1000);
  
  // Build the message to transmit
  msg = "{";
  msg += DHT11.humidity * 100;
  msg += ",";
  msg += DHT11.temperature * 100;
  msg += ",";
  msg += int(loadVoltage * 1000);
  msg += ",";
  msg += int(current_mA);
  msg += "}";

  /*
  Serial.print("Bus Voltage:   "); Serial.print(busVoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntVoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadVoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.println("");
  */

  Serial.println(msg);

  if (loadVoltage > 2 && loadVoltage < 12.36 && powerOnSeconds > 60) {
    // If V < 11.8V, battery is drained. TESTING! :-)
    lightOff();
    powerOff();
  } else {
    if (Serial.available() > 0) {
      Serial.print("Incoming character...");
      inByte = Serial.read();
      Serial.println(inByte);
  
      switch (inByte) {
      case 'A':
        // Enable automatic power/light management
        isAutonomous = true;
        break;
      case 'a':
        // Disable automatic power/light management
        isAutonomous = false;
        break;
      default:
        if (!isAutonomous) {  // Only act on inputs if isAutonomous is overridden
          actOnInput(inByte);
        }
        break;
      }    
    }
    
    if (isAutonomous) {
      if (DHT11.temperature > 1 && DHT11.temperature < 90) {
        // Temperature is within desired range...
        // Extinguish power (heat/fan), ignite "ready" light.
        powerOff();
        lightOn();
      } else {
        if (loadVoltage > 12.57) {
          // Temperature is out of desired range...
          // Engage heat/fan (depending upon season) and extinguish light.
          // If V too low, though, it can't sustain the heater power draw.
          powerOn();
          lightOff();
        } else {
          if (powerOnSeconds > 60) {
            powerOff();
            lightOn();
          } else {
            powerOnSeconds++;
          }
        }
      }
    }
  }
  
  delay(1000);
}

void lightOn() {
  // Turn on light (if not on already)
  if (!isLightOn) {
    digitalWrite(lightPin, HIGH);
    isLightOn = true;
  }
}

void lightOff() {
  // Turn off light (if on)
  if (isLightOn) {
    digitalWrite(lightPin, LOW);
    isLightOn = false;
  }
}

void powerOn() {
  // Turn on power (if not on already)
  if (!isPowerOn) {
    digitalWrite(powerPin, HIGH);
    isPowerOn = true;
  }
  powerOnSeconds++;  // increment power on counter
}

void powerOff() {
  // Turn off power (if on)
  if (isPowerOn) {
    digitalWrite(powerPin, LOW);
    isPowerOn = false;
  }
  powerOnSeconds = 0;  // reset counter for time power is on
}

void actOnInput(int inByte) {
  switch (inByte) {
  case 'L':
    lightOn();
    break;
  case 'l':
    lightOff();
    break;
  case 'P':
    powerOn();
    break;
  case 'p':
    powerOff();
    break;
  }    
}
