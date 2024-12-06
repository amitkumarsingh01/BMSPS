#include <SPI.h>
#include <Wire.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "Amit";
const char* password = "12345678";

const String apiKey = "44KHEVR7MG48QPAP";
const char* thingSpeakServer = "api.thingspeak.com";

#define DHT11PIN 18
#define DHTTYPE DHT11
#define RELAY_PIN 26
#define VOLTAGE_SENSOR_PIN 34
#define CURRENT_SENSOR_PIN 32

DHT dht(DHT11PIN, DHTTYPE);

int adc_max = 760;
int adc_min = 261;
float volt_multi = 231.0;
float volt_multi_p;
float volt_multi_n;

int mVperAmp = 185;  

AsyncWebServer server(80);

bool relayState = false;

void setup() {
  Serial.begin(9600);

  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); 

  volt_multi_p = volt_multi * 1.4142; 
  volt_multi_n = -volt_multi_p; 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><title>Relay Control</title><style>body { font-family: Arial, sans-serif; }</style></head><body><h1>Relay Control</h1>";
    html += "<label for='relayToggle'>Relay</label>";
    html += "<input type='checkbox' id='relayToggle' onclick='toggleRelay()' ";
    if (relayState) {
      html += "checked";
    }
    html += "><br>";
    html += "<script>function toggleRelay() {var x = document.getElementById('relayToggle');fetch('/toggle?relayState=' + (x.checked ? 1 : 0));}</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    String state = request->getParam("relayState")->value();
    relayState = (state == "1");
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    request->send(200, "text/plain", "Relay toggled");
  });

  server.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    float voltage = get_voltage();   
    float current = get_current();  

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Voltage: ");
    Serial.print(voltage, 2);
    Serial.println(" V");

    Serial.print("Current: ");
    Serial.print(current, 2);
    Serial.println(" A");

    sendDataToThingSpeak(temperature, humidity, voltage, current);

    delay(10000);
  } else {
    Serial.println("WiFi Disconnected! Reconnecting...");
    WiFi.begin(ssid, password);
  }
}

void sendDataToThingSpeak(float temperature, float humidity, float voltage, float current) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "http://api.thingspeak.com/update?api_key=" + apiKey;
    url += "&field1=" + String(temperature);
    url += "&field2=" + String(humidity);
    url += "&field3=" + String(voltage, 2);
    url += "&field4=" + String(current, 2);

    Serial.println("Sending data to ThingSpeak...");
    Serial.println(url);

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      Serial.println("Data sent to ThingSpeak successfully!");
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected! Unable to send data to ThingSpeak.");
  }
}

float get_voltage() {
  float adc_sample;
  float volt_inst = 0;
  float sum = 0;
  float volt_rms;
  long init_time = millis();
  int N = 0;

  while ((millis() - init_time) < 500) { 
    adc_sample = analogRead(VOLTAGE_SENSOR_PIN);
    volt_inst = map(adc_sample, adc_min, adc_max, volt_multi_n, volt_multi_p);
    sum += sq(volt_inst); 
    N++;
    delay(1);
  }

  volt_rms = sqrt(sum / N); 
  volt_rms = volt_rms * 2.9 / 4.64; 
  if (volt_rms >= 300) {
    volt_rms = 0; 
  }
  return volt_rms;
}


float get_current() {
  float result;
  int readValue;
  int maxValue = 0;
  int minValue = 4096;

  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000) { 
    readValue = analogRead(CURRENT_SENSOR_PIN);
    maxValue = max(maxValue, readValue); 
    minValue = min(minValue, readValue); 
  }

  result = ((maxValue - minValue) * 3.3) / 4096.0; 
  float VRMS = (result / 2.0) * 0.707;  
  float AmpsRMS = ((VRMS * 1000.0) / mVperAmp) - 0.3;

  AmpsRMS *= 2; 
  if (AmpsRMS > 0.70) {
    AmpsRMS = 0; 
  }
  if (AmpsRMS < 0) {
    AmpsRMS = -AmpsRMS; 
  }

  return AmpsRMS;
}
