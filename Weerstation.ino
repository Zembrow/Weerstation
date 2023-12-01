// Include libraries
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <TFT_eSPI.h>

// Include environment variables
#include "env.h"

// Define components
WiFiUDP NTP; // NTPClient
NTPClient timeClient(NTP); // NTPClient
StaticJsonDocument<1024> weatherForecast; //Weather forecast JSON element
Adafruit_BME680 bme; // BME680-Sensor
TFT_eSPI tft = TFT_eSPI(); // LCD screen;

// Global variables
//
// WiFi
String IPURL = "http://api.ipify.org";
String WeatherAPIURL = "http://api.weatherapi.com/v1/current.json?key=" + WeahterAPIKey + "&q=";
String publicIP;

// Weather Forecast
float outsideTemperature;
const char* outsideCondition;
float outsidePrecipitation;

// Weather Forecast units
String outsideTemperatureUnit = "*C";
String outsidePrecipitationUnit = "mm";

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

  // Init LCD screen
  tft.begin();
  tft.setRotation(3);
  tft.invertDisplay(false);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);

  // Loadbar
  tft.drawString("Loading...", tft.getViewportWidth() / 2, tft.getViewportHeight() / 2 - 10 , 2);
  tft.fillRect(tft.getViewportWidth() / 2 - 100, tft.getViewportHeight() / 2 + 10, 200, 25, TFT_WHITE);
  tft.fillRect(tft.getViewportWidth() / 2 - 100 + 2, tft.getViewportHeight() / 2 + 10 + 2, 196, 21, TFT_BLACK);

  tft.setTextDatum(TL_DATUM);

  // Start WiFi connection
  WiFi.begin(WiFiSSID, WiFiPassword);

  int attempts = 0; // Keep track of # of checks

  while (WiFi.status() != WL_CONNECTED) {
    // After 5s, try to connect again
    if (attempts == 5) {
      WiFi.begin(WiFiSSID, WiFiPassword);
      attempts = 0;
    }

    delay(1000);
    attempts++;
  }

  updateLoadbar(25);

  // Init NTPClient
  timeClient.begin();

  updateLoadbar(50);

  // Get WiFi public IP
  publicIP = HTTPRequest(IPURL);

  updateLoadbar(75);

  // Init BME680-sensor
  bme.begin();

  updateLoadbar(100);
  delay(500);
}

// Loop-function
// Function loops continuously 
void loop() {
  // Get current date & time
  // year() month() day() hour() minute() second()
  timeClient.update();
  int epochTime = timeClient.getEpochTime();
  setTime(epochTime);

  // Get current weather & transfrom JSON -> Array
  String WeatherRequest = HTTPRequest(WeatherAPIURL + publicIP);
  deserializeJson(weatherForecast, WeatherRequest);

  // Read weather forecast from request
  JsonObject current = weatherForecast["current"]; // Set correct depth, otherwise ESP will overload

  outsideTemperature = current["temp_c"];
  outsideCondition = current["condition"]["text"];
  outsidePrecipitation = current["precip_mm"];

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
  //Serial.println();
  
  delay(10000000);
}

// HTTP GET Request function
// Makes API request to an API
String HTTPRequest(String URL) {
  // Init
  HTTPClient http;
  http.setTimeout(10000);

  // API Request
  http.begin(URL);
  http.GET();

  // Get reponse
  String response = http.getString();

  // Clear request client
  http.end();

  // Return
  return response;
}

// Update the loading-screen bar
void updateLoadbar(int percentage) {
  // 192 = 100%
  tft.fillRect(tft.getViewportWidth() / 2 - 100 + 4, tft.getViewportHeight() / 2 + 10 + 4, 1.92 * percentage, 17, TFT_WHITE);
}