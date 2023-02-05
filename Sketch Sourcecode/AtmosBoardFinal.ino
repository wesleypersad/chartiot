// include wifi & server libararies
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"
#include "ThingSpeak.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP085.h>
// libraries for OTA
#include <ArduinoOTA.h>

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
StaticJsonDocument<400> doc;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// do the OLED display stuff
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WiFiClient tsclient;

// define the ThingSpeak id & api
#define CHANNEL_ID 1627862
#define CHANNEL_API_KEY "FOD7QX88PR1UBBLE"

// define the temp sensor
#define DHT_SENSOR_TYPE DHT_TYPE_11

// wifi details
#ifndef STASSID
#define STASSID "SKY79BAD"
#define STAPSK  "YTDAUPSF"
#endif

// set the port for the wifi server
ESP8266WebServer server(80);

// define wifi login
const char* ssid = STASSID;
const char* password = STAPSK;

int minTemp = -2;
int maxTemp = 6;

// time variables to allow webpage requests
int period = 100;
unsigned long time_now = 0;

// init temp & humidity pin
const int temp_hum_pin = D3;

// init the DHT11 component
DHT dht(temp_hum_pin, DHT11);

// define the bmp object
Adafruit_BMP085 bmp;

// init the variables to store temperature & humidity
float temperature = 0.0;
float avgTemperature = 0.0;
float humidity = 0.0;
float avgHumidity = 0.0;

//pressure sensor
float pressure = 0.0;
float avgPressure = 0.0;

// gas sensor inputs and values
int sendigpin = D0;
int senanlpin = A0;
//uint16_t gasVal;
float gasVal = 0.0;
float avgGasVal = 0.0;
boolean isgas = false;
String gas;

// set the debug flag for messages
static bool debugOn = true;

// initialise  const int called 'ledpin' to pin D5
const int ledPin = D5;

// define the hi alarm for gas ppm
float hiGasppm = 3000.0;
bool gasAlarm = false;

// define hi alarm fro humidity
float hiHumidity = 70.0;
bool humAlarm = false;

// define the alarm deadbands to prevent alarm bounce
float alarmProp = 0.01;               // 1% of instrument range
float gasDB = alarmProp * 1000000.0;  // range ppm
float humDB = alarmProp * 100.0;      // 0-100%

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

  // strt reading dht component
  dht.begin();

  // initialise the OLED display and say Hello world
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    if (debugOn) Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Hello, world!");
  display.display();

  // initialise the bmp sensor
  if (!bmp.begin()) {
    if (debugOn) Serial.println("Could not find BMP sensor, check wiring!");
    while (1) {}
  }

  // setup sensor pins
  pinMode(sendigpin, INPUT);

  // Add values in the document
  doc["content"] = "sensors";
  doc["time"] = 1351824120;
  
  JsonArray sensors = doc.createNestedArray("sensors");
  
  JsonObject sensors_0 = sensors.createNestedObject();
  sensors_0["type"] = "temperature";
  sensors_0["value"] = 0;
  sensors_0["units"] = "degC";
  
  JsonObject sensors_1 = sensors.createNestedObject();
  sensors_1["type"] = "humidity";
  sensors_1["value"] = 0;
  sensors_1["units"] = "%";
  
  JsonObject sensors_2 = sensors.createNestedObject();
  sensors_2["type"] = "gasppm";
  sensors_2["value"] = 0;
  sensors_2["units"] = "ppm";
  
  JsonObject sensors_3 = sensors.createNestedObject();
  sensors_3["type"] = "pressure";
  sensors_3["value"] = 0;
  sensors_3["units"] = "Pa";

  // for timeclient
  timeClient.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  // update and write time
  updateValues();

  // handle OTA and webpage requests
  handleOTA();
  
  // read temp & humidity etc
  readTempHumEtc();

  // displat the T & H
  dispValues();

  // display values from bmp
  dispbmp();

  // dispaly the sensor value
  dispsensor();

  // check alarms
  checkAlarms();

  // update the averages
  calcAvgs(); 

  // write thing values
  writeThingVals();
}

void dispsensor() {
  gasVal = analogRead(senanlpin);
  isgas = digitalRead(sendigpin);
  if (isgas) {
    gas = "No";
  }
  else {
    gas = "Yes";
  }
  // the range of the gas sensor is 0 to 10000ppm, changed from %
  gasVal = mapfloat(gasVal, 0, 1023, 0, 10000);
  //Serial.print("Gas detected: ");
  //Serial.print(gas);
  //Serial.print("\t");
  //Serial.print("Gas ppm: ");
  //Serial.println(gasVal);
  //Serial.println(gasAlarm);
}

void dispbmp() {
  // set variable for pressure
  pressure = bmp.readPressure();
  //Serial.print("BMP Temperature = ");
  //Serial.print(bmp.readTemperature());
  //Serial.print(" *C");
  //Serial.print("\t");
  //Serial.print("BMP Pressure = ");
  //Serial.print(bmp.readPressure());
  //Serial.println(" Pa");  
}

void dispValues() {
  //read temperature and humidity
  float t = temperature;
  float h = humidity;
  float g = gasVal;
  
  //clear display
  display.clearDisplay();

  // display temperature
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Temperature: ");
  display.setTextSize(2);
  display.setCursor(0,10);
  display.print(t);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");
  
  // display humidity
  //display.setTextSize(1);
  //display.setCursor(0, 35);
  //display.print("Humidity: ");
  //display.setTextSize(2);
  //display.setCursor(0, 45);
  //display.print(h);
  //display.print(" %"); 

  // display gas value
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("Gas %: ");
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(g);
  display.print(" %"); 
  
  display.display();   
}

void readTempHumEtc() {
  int tempVal = 0;
  tempVal = dht.readTemperature();
  if (tempVal >=0 && tempVal <= 50) {
    temperature = tempVal;
  }
  tempVal = dht.readHumidity();
  if (tempVal >= 0 && tempVal <= 100) {
    humidity = tempVal;
  }

  //Serial.print("Temperatue: ");
  //Serial.print(temperature);
  //Serial.print("\t");
  //Serial.print("Humidity: ");
  //Serial.print(humidity);
  //Serial.print("\t");
}

void get_index() {
  // print the welcome text on the index page

  String html ="<!DOCTYPE html> <html> ";
  html+= "<head><meta http-equiv='refresh' content='15'></head>";
  html+= "<body> <h1>WVP Atmos Board OTA Dashboard</h1>";
  html+= "<p> Welcome to the ATMOS dashboard</p>";
  html+= "<div> <p> <strong> The temperature reading is: ";
  html+= temperature;
  html+= "</strong> degrees.</p>";
  html+= "<div> <p> <strong> The humidity reading is: ";
  html+= humidity;
  html+= "</strong> % ";
  html+= humAlarm ? " ALARM":" NORMAL";
  html+= "</p> </div>";
  html+= "<div> <p> <strong> The atmospheric pressure reading is: ";
  html+= pressure;
  html+= "</strong> Pa </p>";
  html+= "<div> <p> <strong> The Gas ppm reading is: ";
  html+= gasVal;
  html+= "</strong> ppm ";
  html+= gasAlarm ? " ALARM":" NORMAL";
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

void handleOTA() {
  // handle the OTA stuff  
  ArduinoOTA.handle();
  // handle the incoming client requests
  server.handleClient();
}

void checkAlarms() {
  // see if humidity in alarm
  if (humidity > hiHumidity) {    
    humAlarm = true;
    // adjust alarm to prevent bounce
    hiHumidity -= humDB;
  } else {
    humAlarm = false;
    // reset the alarm
    hiHumidity += humDB; 
  };
  // see if the gas ppm is in alarm
  if (gasVal > hiGasppm) {
    gasAlarm = true;
    // reset the alarm
    hiGasppm -= gasDB; 
  } else {
    gasAlarm = false;
    // reset the alarm
    hiGasppm += gasDB;
  }; 
  // if any alarm, LED On else LED off
  if (humAlarm || gasAlarm) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  };
}

void calcAvgs() {
  static unsigned long avgMillis = millis();

  // non-blocking average every 1 second i.e. 60 values
  // averaged for a minute average value
  if (millis() - avgMillis > 1000) {
    avgTemperature += (temperature/60.0);
    avgHumidity += (humidity/60.0);
    avgGasVal += (gasVal/60.0);
    avgPressure += (pressure/60.0);
    avgMillis = millis();  
  } 
}

void writeThingVals() {
  static unsigned long writeMillis = millis();

  // non-blocking routine to only write after 60 seconds
  if (millis() - writeMillis > 60000) {
    //write the values to ThingSpeak
    ThingSpeak.setField(1, avgTemperature);
    ThingSpeak.setField(2, avgHumidity);
    ThingSpeak.setField(3, humAlarm);
    ThingSpeak.setField(5, avgGasVal);
    ThingSpeak.setField(6, avgPressure);
    ThingSpeak.setField(7, gasAlarm);
    ThingSpeak.writeFields(CHANNEL_ID, CHANNEL_API_KEY);
    // reset averages to zero
    avgTemperature = 0.0;
    avgHumidity = 0.0;
    avgGasVal = 0.0;
    avgPressure = 0.0;    
    writeMillis = millis();
  }
}

void updateValues() {
  //update time, values etc
  timeClient.update();
  doc["time"] = timeClient.getEpochTime();
  doc["sensors"][0]["value"] = temperature;
  doc["sensors"][1]["value"] = humidity;
  doc["sensors"][2]["value"] = gasVal;
  doc["sensors"][3]["value"] = pressure;
}
