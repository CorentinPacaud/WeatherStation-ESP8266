/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com
*********/

// Including the ESP8266 WiFi library
#include <ESP8266WiFi.h>
#include "DHT.h"
#include "config.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <math.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Replace with your network details
const char* ssid = SSID;
const char* password = SSID_PASSWORD;

// Web Server on port 80
WiFiServer server(80);

// DHT Sensor
#define DHTPin D3
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// SCREEN
#define OLED_RESET 2
Adafruit_SSD1306 display(OLED_RESET);


// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];
// Weather for Lyon
char weatherUrl[]  = OPENWEATHER_URL;

// TIME
char timeUrl[] = TIME_URL;

// THINGSpeak
String TSPost = THINGSPEAK_POST;
String TSGetLastFiel2 = THINGSPEAK_GETLAST_FIELD2;

HTTPClient http;
bool weatherB = false;
String weather = "X";
float extTemp = 14;
float hic = 20.0;
String cNow = "19:02";
int sleepSec = 0;

int currentScreen = 0;

unsigned long millisScreens = 0;
unsigned long millisTime = 0;
unsigned long millisTemp = 0;

// only runs once on boot
void setup() {
  // Initializing serial port for debugging purposes
  Serial.begin(115200);
  delay(10);

  dht.begin();

  // Connecting to WiFi network
  initWifi();

  Serial.println("Init display");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();
  Serial.println("End of init.........");
}



// runs over and over again
void loop() {
  unsigned long currentMillis = millis();

  // EVERY MINUTE -> TODO CHANGE TO ONCE A DAY
  if ((unsigned long)(currentMillis - millisTime) >= (1000 * 60) || millisTime == 0) {
    getTime();
    millisTime = currentMillis;
  }

  // EVERY 5 min
  if ((unsigned long)(currentMillis - millisTemp) >= (1000 * 60 * 5) || millisTemp == 0) {
    getTemperature();
    millisTemp = currentMillis;
  }

  // EVERY 5s
  if ((unsigned long)(currentMillis - millisScreens) >= (1000 * 5) || millisScreens == 0) {
    showDisplay2();
    millisScreens = currentMillis;
  }
}

void getTemperature() {
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  }
  else {
    // Computes temperature values in Celsius + Fahrenheit and Humidity
    hic = dht.computeHeatIndex(t, h, false);
  }

  // POST TEMP
  http.begin(TSPost+"&field1="+hic);
  int httpCode = http.GET();   //Send the request
  if (httpCode == 200) {
    // SUCCESS
    Serial.println("POST T SUCCESS");
  } else {
    Serial.println("POST T Error : " + httpCode);
  }
  http.end();  //Close connection

  // GET EXT TEMP
  http.begin(TSGetLastFiel2);
  httpCode = http.GET();
  if (httpCode > 0) { //Check the returning code
    String json = http.getString();   //Get the request response payload
    Serial.println(json);                     //Print the response payload
    const size_t bufferSize = JSON_OBJECT_SIZE(3) + 70;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.parseObject(json);

    const char* created_at = root["created_at"]; // "2018-05-14T20:41:46Z"
    int entry_id = root["entry_id"]; // 624
    const char* field2 = root["field2"]; // "19.6"
    extTemp = atof(field2);
  } else {
    Serial.println("GET xT Error : " + httpCode);
  }
  http.end();   //Close connection

}

void getWeather() {
  Serial.println("Get Weather");
  http.begin(weatherUrl);
  int httpCode = http.GET();   //Send the request
  String res = "X";

  if (httpCode > 0) { //Check the returning code
    String json = http.getString();   //Get the request response payload
    Serial.println(json);                     //Print the response payload
    const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(12) + 390;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success()) {
      Serial.println("Get Weather ERROR");
    }
    int cod = root["cod"]; // 200
    if (cod == 200) {
      JsonObject& weather0 = root["weather"][0];
      int weather0_id = weather0["id"]; // 800
      const char* weather0_main = weather0["main"]; // "Clear"
      weather =  String(weather0_main[0]);
      JsonObject& main = root["main"];
      float main_temp = main["temp"];
      extTemp = main_temp;
      Serial.println("Get Weather: " + weather);
      Serial.println("Get Weather: " + String(extTemp));
    }
  }
  http.end();   //Close connection
}

void getTime() {

  http.begin(timeUrl);
  int httpCode = http.GET();
  if (httpCode > 0) { //Check the returning code
    String json = http.getString();   //Get the request response payload
    Serial.println(json);                     //Print the response payload
    const size_t bufferSize = JSON_OBJECT_SIZE(8) + 280;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.parseObject(json);
    const char* localDate = root["localDate"];
    parseTime( reinterpret_cast<const char*>(localDate));
  }
}

void initWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Starting the web server
  server.begin();
  Serial.println("Web server running. Waiting for the ESP IP...");
  // delay(10000);

  // Printing the ESP IP address
  Serial.println(WiFi.localIP());
}

void showDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("T:");
  display.setTextSize(2);
  display.print(String(round(hic)));
  display.setTextSize(1);
  display.print("*C");
  display.setTextSize(1);
  display.setCursor(64, 0);
  display.print("xT:");
  display.setTextSize(2);
  display.print(String(round(extTemp)));
  display.setTextSize(1);
  display.print("*C");
  display.setTextSize(2);
  display.setCursor(34, 17);
  display.print(cNow);

  display.display();
}

void showDisplay2() {
  if (currentScreen == 0) {
    display.clearDisplay();
    display.setTextSize(4);
    display.setCursor(8, 0);
    display.print(cNow);
    currentScreen = 1;
  } else if (currentScreen == 1) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("T");
    display.setTextSize(4);
    display.setCursor(50, 4);
    display.print(String(round(hic)));
    currentScreen = 2;
  } else if (currentScreen == 2) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("xT");
    display.setTextSize(4);
    display.setCursor(50, 4);
    display.print(String(round(extTemp)));
    currentScreen = 0;
  }

  display.display();
}


void parseTime(String date) {
  int index = date.indexOf(':');
  Serial.println("Date:" + date);
  if (index >= 0) {
    Serial.println("Index:" + String(index));
    char n[5];
    n[0] = date.charAt(index - 2);
    n[1] = date.charAt(index - 1);
    n[2] = ':';
    n[3] = date.charAt(index + 1);
    n[4] = date.charAt(index + 2);
    cNow = String(n);
    sleepSec = (date.charAt(index + 4) - '0') * 10 + (date.charAt(index + 5) - '0');
    Serial.println("SleepSec:" + String(sleepSec));
  }
}

