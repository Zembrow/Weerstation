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
#include <ArduinoMqttClient.h>

// Include environment variables
#include "env.h"

// Include icons
#include "icons_day.h"
#include "icons_night.h"

// Define components
WiFiUDP NTP; // NTPClient
WiFiClient wifiClient; // WiFiClient
MqttClient mqttClient(wifiClient); // MQTTClient
NTPClient timeClient(NTP); // NTPClient
StaticJsonDocument<1024> weatherForecast; //Weather forecast JSON element
Adafruit_BME680 bme; // BME680-Sensor
TFT_eSPI tft = TFT_eSPI(); // LCD screen;
TFT_eSprite sprite = TFT_eSprite(&tft); // Sprite to remove update flicker

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

// Weather icon forecast
int forecastIcon;
bool forecastIsDay;

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

// Date
String correctedDay;
String correctedMonth;
String correctedYear;

// Time
String correctedHour;
String correctedMinute;

// Iconlist
struct icon {
  int reference;
  bool isDay;
  const uint8_t *bitmap;
};

struct icon icons[96];

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
  tft.drawString("LOADING...", tft.getViewportWidth() / 2, tft.getViewportHeight() / 2 - 10);
  tft.fillRect(tft.getViewportWidth() / 2 - 100, tft.getViewportHeight() / 2 + 10, 200, 25, TFT_WHITE);
  tft.fillRect(tft.getViewportWidth() / 2 - 100 + 2, tft.getViewportHeight() / 2 + 10 + 2, 196, 21, TFT_BLACK);

  // Data for structure
  int references[] = {
    1000,
    1003,
    1006,
    1009,
    1030,
    1063,
    1066,
    1069,
    1072,
    1087,
    1114,
    1117,
    1135,
    1147,
    1150,
    1153,
    1168,
    1171,
    1180,
    1183,
    1186,
    1189,
    1192,
    1195,
    1198,
    1201,
    1204,
    1207,
    1210,
    1213,
    1216,
    1219,
    1222,
    1225,
    1237,
    1240,
    1243,
    1246,
    1249,
    1252,
    1255,
    1258,
    1261,
    1264,
    1273,
    1276,
    1279,
    1282};

  // Bitmaps
  const uint8_t *iconBitmaps[] = {
    iconDay1000,
    iconDay1003,
    iconDay1006,
    iconDay1009,
    iconDay1030,
    iconDay1063,
    iconDay1066,
    iconDay1069,
    iconDay1072,
    iconDay1087,
    iconDay1114,
    iconDay1117,
    iconDay1135,
    iconDay1147,
    iconDay1150,
    iconDay1153,
    iconDay1168,
    iconDay1171,
    iconDay1180,
    iconDay1183,
    iconDay1186,
    iconDay1189,
    iconDay1192,
    iconDay1195,
    iconDay1198,
    iconDay1204,
    iconDay1207,
    iconDay1210,
    iconDay1213,
    iconDay1201,
    iconDay1216,
    iconDay1219,
    iconDay1222,
    iconDay1225,
    iconDay1237,
    iconDay1243,
    iconDay1255,
    iconDay1246,
    iconDay1258,
    iconDay1240,
    iconDay1273,
    iconDay1252,
    iconDay1261,
    iconDay1264,
    iconDay1276,
    iconDay1282,
    iconDay1279,
    iconDay1249,
    iconNight1000,
    iconNight1003,
    iconNight1006,
    iconNight1009,
    iconNight1030,
    iconNight1063,
    iconNight1066,
    iconNight1069,
    iconNight1072,
    iconNight1087,
    iconNight1114,
    iconNight1117,
    iconNight1135,
    iconNight1147,
    iconNight1150,
    iconNight1153,
    iconNight1168,
    iconNight1171,
    iconNight1180,
    iconNight1183,
    iconNight1186,
    iconNight1189,
    iconNight1192,
    iconNight1195,
    iconNight1198,
    iconNight1204,
    iconNight1207,
    iconNight1210,
    iconNight1213,
    iconNight1201,
    iconNight1216,
    iconNight1219,
    iconNight1222,
    iconNight1225,
    iconNight1237,
    iconNight1243,
    iconNight1255,
    iconNight1246,
    iconNight1258,
    iconNight1240,
    iconNight1273,
    iconNight1252,
    iconNight1261,
    iconNight1264,
    iconNight1276,
    iconNight1282,
    iconNight1279,
    iconNight1249};

  // Fill structure
  bool isDay = true;

  int y = 0;

  // Fill struct
  for (int i = 0; i < 96; i++) {

    // Reset for night icons
    if (i == 49) {
      isDay = false;
      y = 0;
    }

    // WeatherAPI reference
    icons[i].reference = references[y];

    // Is day True/False
    icons[i].isDay = isDay;

    // Icon bitmap
    icons[i].bitmap = iconBitmaps[i];

    y++;
  }

  updateLoadbar(14.3); // Update loadbar

  // Start WiFi connection
  WiFi.begin(WiFiSSID, WiFiPassword);

  int wifiAttempts = 0; // Keep track of # of checks

  while (WiFi.status() != WL_CONNECTED) {
    // After 5s, try to connect again
    if (wifiAttempts == 5) {
      WiFi.begin(WiFiSSID, WiFiPassword);
      wifiAttempts = 0;
    }

    delay(1000);
    wifiAttempts++;
  }

  updateLoadbar(28.6); // Update loadbar

  // Start MQTT connecting to broker
  mqttClient.setUsernamePassword(MQTTUsername, MQTTPassword);
  mqttClient.connect(MQTTURL, MQTTPort);

  int mqttAttempts = 0; // Keep track of # of checks

  while (mqttClient.connected() != true) {
    // After 5s, try to connect again
    if (mqttAttempts == 5) {
      mqttClient.connect(MQTTURL, MQTTPort);
      mqttAttempts = 0;
    }

    delay(1000);
    mqttAttempts++;
  } 

  updateLoadbar(42.9); // Update loadbar

  // Init NTPClient
  timeClient.begin();

  updateLoadbar(57.1); // Update loadbar

  // Get WiFi public IP
  publicIP = HTTPRequest(IPURL);

  updateLoadbar(71.4);

  // Init BME680-sensor
  bme.begin();

  updateLoadbar(85.7); // Update loadbar

  // Init sprite
  sprite.setColorDepth(8);
  sprite.setTextSize(2);
  sprite.createSprite(320, 240);

  updateLoadbar(100); // Update loadbar

  delay(500);
}

// Loop-function
// Function loops continuously 
void loop() {
  // Get current date & time
  timeClient.update();
  int epochTime = timeClient.getEpochTime();
  setTime(epochTime);

  // Date
  correctedDay = beautifyDateTime(day());
  correctedMonth = beautifyDateTime(month());
  correctedYear = beautifyDateTime(year());

  // Time
  correctedHour = beautifyDateTime(hour());
  correctedMinute = beautifyDateTime(minute());

  // Get current weather & transfrom JSON -> Array
  String WeatherRequest = HTTPRequest(WeatherAPIURL + publicIP);
  deserializeJson(weatherForecast, WeatherRequest);

  // Read weather forecast from request
  JsonObject current = weatherForecast["current"]; // Set correct depth, otherwise ESP will overload

  forecastIcon = current["condition"]["code"];

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

  // Update screen with latest data
  loadScreen();

  // Update MQTT Broker with latest data
  updateMQTTBroker();

  // [FOR DEBUGGING ONLY] Print to serial channel
  //Serial.println();
  
  delay(60000 - (second() * 1000));
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

// Load data to screen using a sprite
// Sprites remove update flicker yet use a lot of RAM
void loadScreen() {
    
  sprite.fillScreen(TFT_BLACK); // Reset screen

  // Lines
  sprite.drawLine(tft.getViewportWidth() / 2, 0, tft.getViewportWidth() / 2, tft.getViewportHeight() - 25, TFT_WHITE); // Vertical line
  sprite.drawLine(0, tft.getViewportHeight() - 25, tft.getViewportWidth(), tft.getViewportHeight() - 25, TFT_WHITE); // Bottom horizontal line

  // In- outdoor
  sprite.setTextDatum(TL_DATUM);

  sprite.drawString("IN", 5, 5, 1);;
  sprite.drawRightString("OUT", tft.getViewportWidth() - 5, 5, 1);

  sprite.setTextDatum(MC_DATUM);

  // Date & Time
  String date = String(correctedDay + "/" + correctedMonth + "/" + correctedYear);
  String time = String(correctedHour + ":" + correctedMinute);

  sprite.drawString(date, tft.getViewportWidth() / 4, tft.getViewportHeight() - 10);  // Draw date
  sprite.drawString(time, tft.getViewportWidth() / 4 * 3, tft.getViewportHeight() - 10); // Draw time

  // Indoor
  sprite.drawString(String(insideTemperature) + insideTemperatureUnit, tft.getViewportWidth() / 4, (tft.getViewportHeight() - 25) / 5, 1);
  sprite.drawString(String(insideHumidity) + insideHumidityUnit, tft.getViewportWidth() / 4, (tft.getViewportHeight() - 25) / 5 * 2, 1);
  sprite.drawString(String(insidePressure) + insidePressureUnit, tft.getViewportWidth() / 4, (tft.getViewportHeight() - 25) / 5 * 3, 1);
  sprite.drawString(String(insideGas) + insideGasUnit, tft.getViewportWidth() / 4, (tft.getViewportHeight() - 25) / 5 * 4, 1);

  // icon
  sprite.drawBitmap((tft.getViewportWidth() / 4) * 3 - 32, (tft.getViewportHeight() - 25 - 64) / 5, findIcon(), 64, 64, TFT_WHITE);
  sprite.drawString(String(outsideCondition), tft.getViewportWidth() / 4 * 3, (tft.getViewportHeight() - 25) / 5 * 2.5, 1);

  // Outdoor
  sprite.drawString(String(outsideTemperature) + outsideTemperatureUnit, tft.getViewportWidth() / 4 * 3, 150, 1);
  sprite.drawString(String(outsidePrecipitation) + outsidePrecipitationUnit, tft.getViewportWidth() / 4 * 3, 175, 1);

  sprite.pushSprite(0, 0);
}

// Send data to MQTT Broker
void updateMQTTBroker() {
  mqttClient.poll(); // MQTTClient keep alive

  // Send all data
  updateMQTT("Freark/inside/temperature", String(insideTemperature));
  updateMQTT("Freark/inside/humidity", String(insideHumidity));
  updateMQTT("Freark/inside/pressure", String(insidePressure));
  updateMQTT("Freark/inside/gas", String(insideGas));
  
  updateMQTT("Freark/outside/temperature", String(outsideTemperature));
  updateMQTT("Freark/outside/precipitation", String(outsidePrecipitation));
}

// Create MQTT messages
void updateMQTT(String topic, String value) {
  mqttClient.beginMessage(topic, true, 0);
  mqttClient.print(value);
  mqttClient.endMessage();
}

// Find & return correct icon
const uint8_t *findIcon() {
  for (int i = 0; i < 96; i++) {
    if (icons[i].reference == forecastIcon && icons[i].isDay == forecastIsDay) {
      return icons[i].bitmap;
    }
  }
}

// Convert date/time to a string and add a 0 if number < 10
String beautifyDateTime(int number) {
  if (number < 10)
    return "0" + String(number);

  return String(number);
}