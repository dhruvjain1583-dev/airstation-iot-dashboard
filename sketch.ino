#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ2_AO 34
#define MQ2_DO 35
#define LDR_AO 33
#define LDR_DO 32
#define LED_PIN 2

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "dhruv/airstation/data";
const char* mqtt_alert_topic = "dhruv/airstation/alert";

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient espClient;
PubSubClient client(espClient);

float temperature = 0;
float humidity = 0;
int gasValue = 0;
int lightValue = 0;
bool gasAlert = false;
bool gasDO = false;

unsigned long lastSensorRead = 0;
unsigned long lastMQTTPublish = 0;
unsigned long lastOLEDUpdate = 0;
int oledPage = 0;

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32-AirStation-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 3s");
      delay(3000);
    }
  }
}

void readSensors() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT22 read failed!");
    temperature = 0;
    humidity = 0;
  }

  gasValue = analogRead(MQ2_AO);
  gasDO = digitalRead(MQ2_DO);
  lightValue = analogRead(LDR_AO);

  gasAlert = (gasValue > 2000) || (gasDO == LOW);

  digitalWrite(LED_PIN, gasAlert ? HIGH : LOW);

  Serial.println("=== Sensor Readings ===");
  Serial.println("Temp: " + String(temperature) + " C");
  Serial.println("Humidity: " + String(humidity) + " %");
  Serial.println("Gas AO: " + String(gasValue));
  Serial.println("Gas DO: " + String(gasDO));
  Serial.println("Light: " + String(lightValue));
  Serial.println("Alert: " + String(gasAlert));
}

void publishMQTT() {
  if (!client.connected()) connectMQTT();

  StaticJsonDocument<256> doc;
  doc["temperature"] = round(temperature * 10) / 10.0;
  doc["humidity"] = round(humidity * 10) / 10.0;
  doc["gas_raw"] = gasValue;
  doc["light_raw"] = lightValue;
  doc["gas_alert"] = gasAlert;
  doc["device"] = "ESP32-AirStation";

  char payload[256];
  serializeJson(doc, payload);
  client.publish(mqtt_topic, payload);
  Serial.println("MQTT published: " + String(payload));

  if (gasAlert) {
    StaticJsonDocument<128> alertDoc;
    alertDoc["alert"] = "GAS_DETECTED";
    alertDoc["gas_raw"] = gasValue;
    alertDoc["temperature"] = temperature;
    char alertPayload[128];
    serializeJson(alertDoc, alertPayload);
    client.publish(mqtt_alert_topic, alertPayload);
    Serial.println("ALERT published!");
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (oledPage == 0) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("== Air Station ==");

    display.setCursor(0, 14);
    display.print("Temp: ");
    display.print(temperature, 1);
    display.println(" C");

    display.setCursor(0, 26);
    display.print("Humidity: ");
    display.print(humidity, 1);
    display.println(" %");

    display.setCursor(0, 38);
    display.print("Gas: ");
    display.print(gasValue);
    display.print(" | Light: ");
    display.println(lightValue);

    display.setCursor(0, 52);
    if (gasAlert) {
      display.setTextSize(1);
      display.println("!! GAS ALERT !!");
    } else {
      display.println("Status: NORMAL");
    }
  }

  else if (oledPage == 1) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("== Network ==");

    display.setCursor(0, 14);
    if (WiFi.status() == WL_CONNECTED) {
      display.println("WiFi: CONNECTED");
      display.setCursor(0, 26);
      display.println(WiFi.localIP().toString());
    } else {
      display.println("WiFi: DISCONNECTED");
    }

    display.setCursor(0, 40);
    if (client.connected()) {
      display.println("MQTT: CONNECTED");
    } else {
      display.println("MQTT: DISCONNECTED");
    }

    display.setCursor(0, 52);
    display.println("Broker: HiveMQ");
  }

  display.display();
  oledPage = (oledPage + 1) % 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(MQ2_DO, INPUT);
  pinMode(LDR_DO, INPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed!");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 24);
  display.println("Air Station v1.0");
  display.display();
  delay(2000);

  connectWiFi();

  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(60);
  connectMQTT();

  Serial.println("Setup complete!");
}

void loop() {
  if (!client.connected()) connectMQTT();
  client.loop();

  unsigned long now = millis();

  if (now - lastSensorRead >= 2000) {
    lastSensorRead = now;
    readSensors();
  }

  if (now - lastMQTTPublish >= 5000) {
    lastMQTTPublish = now;
    publishMQTT();
  }

  if (now - lastOLEDUpdate >= 4000) {
    lastOLEDUpdate = now;
    updateOLED();
  }
}
