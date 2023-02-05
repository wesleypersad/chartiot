// include wifi & server libararies
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "ThingSpeak.h"
#include <Wire.h>
// libraries for OTA
#include <ArduinoOTA.h>
// for ADC
#include <Adafruit_ADS1X15.h>

//for JSON manipulation
#include <ArduinoJson.h>

//for time functions
#include <NTPClient.h>
#include <WiFiUdp.h>

// Allocate the JSON document
//
// Inside the brackets, 200 is the RAM allocated to this document.
// Don't forget to change this value to match your requirement.
// Use arduinojson.org/v6/assistant to compute the capacity.
StaticJsonDocument<300> doc;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


WiFiClient tsclient;

// define the ThingSpeak id & api
#define CHANNEL_ID 1976934
#define CHANNEL_API_KEY "M4Q2BX7R1ZWXV6P8"

// wifi details
#ifndef STASSID
#define STASSID "SKY79BAD"
#define STAPSK  "YTDAUPSF"
#endif

// set the port for the wifi server
ESP8266WebServer server(80);

// define wifi login and MQTT
const char* ssid = STASSID;
const char* password = STAPSK;

// time variables to allow webpage requests
int period = 100;
unsigned long time_now = 0;

// define the water level value in % and minute average
//int levelpin = A0;
float level = 0.0;
float avgLevel = 0.0;

// define the turbidity value and minute average
float turbidity = 0.0;
float avgTurbidity = 0.0;

// define tubidity hi alarm
float hiTurbidity = 250.0;
bool turAlarm = false;

// set the debug flags for messages
static bool debugOn = true;

// initialise  const int called 'ledpin' to pin D0
const int ledPin = D0;

// define the hi & low alarms for level
float hiLev = 80.0;
float lowLev = 20.0;
bool levAlarm = false;

// define the alarm deadbands to prevent alarm bounce
float alarmProp = 0.01;               // 1% of instrument range
float turDB = alarmProp * 1000000.0;  // range ppm
float levDB = alarmProp * 100.0;      // 0-100%

//Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

//Defines map function to return floats
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {
  // set up the alarm led pin
  pinMode(ledPin, OUTPUT);
  
  // try to start the serial link
  Serial.begin(115200);
  
  // start serial to debug values
  if (debugOn) {    
    while(!Serial);
    Serial.println("COMx Available !!!");
  }

  // connect to the wifi network
  WiFi.begin(ssid, password);

  // start of OTA block commands
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
  });
  ArduinoOTA.onEnd([]() {
    if (debugOn) Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (debugOn) Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (debugOn) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    }
  });
  ArduinoOTA.begin();
  if (debugOn) {
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  // end of OTA block
  
  //wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (debugOn)Serial.print("Waiting to connect...");
  }

  // print the board IP address
  if (debugOn) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // update the server details
  server.on("/", get_index);
  server.on("/api/sensors", get_json);
  server.begin();
  if (debugOn)Serial.println("Server listening");

  //Start up the ThingSpeak
  ThingSpeak.begin(tsclient);

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  // Add values in the document
  doc["type"] = "sensors";
  doc["time"] = 1351824120;

  JsonArray sensors = doc.createNestedArray("sensors");
  
  JsonObject sensors_0 = sensors.createNestedObject();
  sensors_0["type"] = "level";
  sensors_0["value"] = 0;
  sensors_0["units"] = "%";
  
  JsonObject sensors_1 = sensors.createNestedObject();
  sensors_1["type"] = "turbidity";
  sensors_1["value"] = 0;
  sensors_1["units"] = "ppm";

  // for timeclient
  timeClient.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  // update and write time
  // update and write time
  updateValues();

  // handle OTA and webpage requests
  handleOTA();

  // read sensor values i.e. temp & humidity etc
  readTempHumEtc();

  // check alarms
  checkAlarms();

  // update the averages
  calcAvgs();

  // write things values
  writeThingVals();
}

void get_index() {
  // print the welcome text on the index page

  String html ="<!DOCTYPE html> <html> ";
  html+= "<head><meta http-equiv='refresh' content='15'></head>";
  html+= "<body> <h1>WVP Liquid Board OTA Dashboard</h1>";
  html+= "<p> Welcome to the LIQUID dashboard</p>";
  html+= "<div> <p> <strong> The turbidity reading is: ";
  html+= turbidity;
  html+= "</strong> ppm. ";
  html+= turAlarm ? " ALARM":" NORMAL";
  html+= "</p> </div>";
  html+= "<div> <p> <strong> The level reading is: ";
  html+= level;
  html+= "</strong> % ";
  html+= levAlarm ? " ALARM":" NORMAL";
  html+= "</p> </div>";
  html+= "<div> <p> <strong> Debug Messages: ";
  html+= debugOn ? "AVAILABLE":"NOT AVAILABLE";
  html+="</body> </html>";

  server.send(200, "text/html", html);
}

void get_json() {
  // print the JSON file
  String json;

  serializeJson(doc, json);

  server.send(200, "text/json", json);
}

void readTempHumEtc() {
  // read and display the water level and tubidity from ADC channels
  int16_t adc0, adc1;
  float volts0, volts1;

  // get voltags from ADC channels 0 & 1
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);

  volts0 = ads.computeVolts(adc0);
  volts1 = ads.computeVolts(adc1);

//  if (debugOn) {
//    Serial.print("Volts0: ");
//    Serial.println(volts0);  
//    Serial.print("Volts1: ");
//    Serial.println(volts1);
//  };  

  // the water level comes into ADC channel 1, where 0-3.3V maps to 0-100%
  // update on level calibration : 0% = 0.30 & 100% = 1.40
  level = mapfloat(volts1, 0.3, 1.4, 0, 100);
  if (level > 100) {
    level = 100.0;
  };
  if (level < 0) {
    level = 0.0;
  };

  // change this to the ADC channel 0, where 0-3.3v maps to 0-1000ppm as before
  turbidity = mapfloat(volts0, 0, 3.3, 0, 1000);

//  if (debugOn) {
//    Serial.print("Water Level: ");
//    Serial.print(level);
//    Serial.print("\t");
//    Serial.print("Turbidity: ");
//    Serial.println(turbidity);
//  };   
}

void handleOTA() {
  // handle the OTA stuff  
  ArduinoOTA.handle();
  // handle the incoming client requests
  server.handleClient();
}

void checkAlarms() {
  // check turbidity
  if (turbidity > hiTurbidity) {
    digitalWrite(ledPin, HIGH);
    turAlarm = true;
    // adjust alarm to prevent bounce
    hiTurbidity -= turDB;
  } else {
    digitalWrite(ledPin, LOW);
    turAlarm = false;
    // reset the alarm
    hiTurbidity += turDB;
  };
  // see if the level is in alarm
  if ((level < lowLev) || (level > hiLev)) {
    digitalWrite(ledPin, HIGH);
    levAlarm = true;
    // adjust alarms to prevent bounce
    hiLev -= levDB;
    lowLev += levDB;
  } else {
    digitalWrite(ledPin, LOW);
    levAlarm = false;
    // reset the alarms
    hiLev += levDB;
    lowLev -= levDB;
  };  
}

void calcAvgs() {
  static unsigned long avgMillis = millis();

  // non-blocking average every 1 seconds i.e. 60 values
  // averaged for a minute average value
  if (millis() - avgMillis > 1000) {
    avgLevel += (level/60.0);
    avgTurbidity += (turbidity/60.0);
    avgMillis = millis();  
  }
}

void writeThingVals() {
  static unsigned long writeMillis = millis();

  // non-blocking routine to only write after 60 seconds
  if (millis() - writeMillis > 60000) {
    ThingSpeak.setField(1, avgLevel);
    ThingSpeak.setField(2, avgTurbidity);
    ThingSpeak.setField(3, levAlarm);
    ThingSpeak.setField(4, turAlarm);
    ThingSpeak.writeFields(CHANNEL_ID, CHANNEL_API_KEY);
    // reset averages to zero
    avgLevel = 0.0;
    avgTurbidity = 0.0;
    writeMillis = millis();
  }
}

void updateValues() {
  //update time, values etc
  timeClient.update();
  doc["time"] = timeClient.getEpochTime();
  doc["sensors"][0]["value"] = level;
  doc["sensors"][1]["value"] = turbidity;
}
