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
  sprite.drawString("IN", 5, 5, 1);;
  sprite.drawRightString("OUT", tft.getViewportWidth() - 5, 5, 1);

  // Date & Time
  String date = String(day()) + "/" + String(month()) + "/" + String(year());
  String time = String(hour()) + ":" + String(minute());

  sprite.drawCentreString(date, tft.getViewportWidth() / 4, tft.getViewportHeight() - 20, 1); // Draw date
  sprite.drawCentreString(time, tft.getViewportWidth() / 4 * 3, tft.getViewportHeight() - 20, 1); // Draw time

  // Indoor
  sprite.drawCentreString(String(insideTemperature) + insideTemperatureUnit, tft.getViewportWidth() / 4, (tft.getViewportHeight() - 25) / 6, 1); 
  sprite.drawCentreString(String(insideHumidity) + insideHumidityUnit, tft.getViewportWidth() / 4, (tft.getViewportHeight() - 25) / 6 * 2, 1);
  sprite.drawCentreString(String(insidePressure) + insidePressureUnit, tft.getViewportWidth() / 4, (tft.getViewportHeight() - 25) / 6 * 3, 1);
  sprite.drawCentreString(String(insideGas) + insideGasUnit, tft.getViewportWidth() / 4, (tft.getViewportHeight() - 25) / 6 * 4, 1);

  // Outdoor
  sprite.drawCentreString(String(outsideTemperature) + outsideTemperatureUnit, tft.getViewportWidth() / 4 * 3, 25, 1); 
  sprite.drawCentreString(String(outsidePrecipitation) + outsidePrecipitationUnit, tft.getViewportWidth() / 4 * 3, 50, 1);

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