// Include libraries
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#include "env.h"

// Define components
Adafruit_BME680 bme; // BME680-Sensor

// Global variables
//
// BME680-sensor readings
float insideTemperature;
float insideHumidity;
float insidePressure;
float insideGas;

// BME680-sensor units
String insideTemperatureUnit = "*C";
String insideHumidityUnit = "%";
String insidePressureUnit = "hPa";
String insideGasUnit = "KOhms"; 

// Setup-function
// Function runs once when ESP32 is booted
void setup() {
  // [FOR DEBUGGING ONLY] Open serial channel
  Serial.begin(115200);

  // Start WiFi connection
  WiFi.begin(WiFiSSID, WiFiPassword);
  delay(10000);

  // Init BME680-sensor
  bme.begin();
}

// Loop-function
// Function loops continuously 
void loop() {
  // Read BME680-sensor
  bme.beginReading();
  delay(50);
  bme.endReading();

  // Read BME680 sensor-data from reading
  insideTemperature = bme.temperature;
  insideHumidity = bme.humidity;
  insidePressure = bme.pressure / 100.0;
  insideGas = bme.gas_resistance / 1000.0;

  // [FOR DEBUGGING ONLY] Print to serial channel
  //Serial.print(insideTemperature + insideTemperatureUnit + insideHumidity + insideHumidityUnit + insidePressure + insidePressureUnit + insideGas + insideGasUnit);
  //Serial.println();
  
  delay(2000);
}