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

HTTPClient http;
bool weatherB = false;
String weather = "X";
float extTemp = 20.0;
float hic = 20.0;
String cNow = "19:02";
int sleepSec = 0;

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
  delay(2000);
}

// runs over and over again
void loop() {
  if (!weatherB) {
    getWeather();
    weatherB = true;
  }
  getTime();
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    strcpy(celsiusTemp, "Failed");
    strcpy(humidityTemp, "Failed");
  }
  else {
    // Computes temperature values in Celsius + Fahrenheit and Humidity
    hic = dht.computeHeatIndex(t, h, false);
    dtostrf(hic, 6, 2, celsiusTemp);
    dtostrf(h, 6, 2, humidityTemp);

  }

  // Listenning for new clients
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New client");
    // bolean to locate when the http request ends
    boolean blank_line = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (c == '\n' && blank_line) {
          // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)

          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          // your actual web page that displays temperature and humidity
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head></head><body><h1>ESP8266 - Temperature and Humidity</h1><h3>Temperature in Celsius: ");
          client.println(celsiusTemp);
          client.println("*F</h3><h3>Humidity: ");
          client.println(humidityTemp);
          client.println("%</h3><h3>");
          client.println("</body></html>");
          break;
        }
        if (c == '\n') {
          // when starts reading a new line
          blank_line = true;
        }
        else if (c != '\r') {
          // when finds a character on the current line
          blank_line = false;
        }
      }
    }
    // closing the client connection
    delay(1);
    client.stop();
    Serial.println("Client disconnected.");
  }
  // Serial.println("Weather: " + weather);
  showDisplay();
  //  Serial.println("Going into deep sleep for 20 seconds");
  ESP.deepSleep(60e6-(sleepSec*1e6)); // 20e6 is 20 microseconds
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

void parseTime(String date) {
  int index = date.indexOf(':');
  Serial.println("Date:" + date);
  if (index >= 0) {
    Serial.println("Index:" + String(index));
    char n[5];
    n[0] = date.charAt(index-2);
    n[1] = date.charAt(index-1);
    n[2] = ':';
    n[3] = date.charAt(index+1);
    n[4] = date.charAt(index+2);
    cNow = String(n);
    sleepSec = (date.charAt(index+4)-'0')*10+(date.charAt(index+5)-'0');
    Serial.println("SleepSec:" + String(sleepSec));
  }
}

