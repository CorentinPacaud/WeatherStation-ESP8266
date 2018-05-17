#include <ESP8266WiFi.h>
#include "DHT.h"
#include <math.h>
#include <ESP8266HTTPClient.h>

#define DHTTYPE DHT11   // DHT 11


const char* ssid = "colocation";
const char* password = "colocationvaise";

#define DHTPin D3
DHT dht(DHTPin, DHTTYPE);
String thingSpeak = "http://api.thingspeak.com/update?api_key=HEGEIZFTN66KVW32";
String getExtTemp = "https://api.thingspeak.com/channels/489319/fields/2.json?api_key=E75SF9QJIJ77J7W6&results=1"

//RTC_DATA_ATTR int bootCount = 0;
HTTPClient http;
float hic = 20.0;

void setup() {
  delay(1000);
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Start of init.........");
  //pinMode(LED_BUILTIN, OUTPUT);
  dht.begin();
  // Connecting to WiFi network
  initWifi();
  Serial.println("End of init.........");
  delay(1000);

  // put your main code here, to run repeatedly:
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
    postTemperature(hic, h);
  }

  //bootCount = bootCount + 1;
//  Serial.println("Boot count : "+ bootCount);
  
  ESP.deepSleep(5*60e6); // 20e6 is 20 microseconds
}

void loop() {
}

// POST TEMP EXT and HUM EXT
void postTemperature(float tempInt, float humExt) {
  //Serial.println("POST Temp");
  String url = thingSpeak + "&field2=" + String(round(tempInt)) + "&field4=" + String(round(humExt));
  //Serial.println("URL : "+url);
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == 200) {
    Serial.println("POST Temp Success");
  } else {
    String response = http.getString();
    Serial.println("POST Temp failed : " + response);
  }
  http.end();
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

  // Printing the ESP IP address
  Serial.println(WiFi.localIP());
}
