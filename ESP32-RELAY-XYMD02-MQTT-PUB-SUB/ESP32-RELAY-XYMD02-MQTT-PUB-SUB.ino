#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <ModbusMaster.h>

// ==== Konfigurasi DHT ====
#define DHTPIN 27
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ==== Konfigurasi Relay ====
#define RELAYPIN1 16
#define RELAYPIN2 17

// ==== Konfigurasi WiFi ====
const char *ssid = "SSID";
const char *password = "PASSWORD";

// ==== Konfigurasi MQTT ====
const char *mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char *mqtt_topic_sub = "topic/relay";
const char *mqtt_topic_pub = "topic/sensor";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

// ==== Konfigurasi MODBUS ====
#define MODBUS_EN_PIN 4  // DE & RE pin
#define MODBUS_RO_PIN 18 // RO pin (RX)
#define MODBUS_DI_PIN 19 // DI pin (TX)
#define MODBUS_SERIAL_BAUD 9600
#define MODBUS_PARITY SERIAL_8N1
#define MODBUS_SLAVE_ID 2
#define MODBUS_ADDRESS 0x0001
#define MODBUS_QUANTITY 2

ModbusMaster modbus;

// ==== Fungsi Modbus TX/RX ====
void modbusPreTransmission()
{
  digitalWrite(MODBUS_EN_PIN, HIGH);
  delay(10);
}
void modbusPostTransmission()
{
  digitalWrite(MODBUS_EN_PIN, LOW);
  delay(10);
}

// ==== Setup WiFi ====
void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi terkoneksi!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());

  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  digitalWrite(RELAYPIN1, LOW);
  digitalWrite(RELAYPIN2, LOW);
}

// ==== Callback MQTT ====
void callback(char *topic, byte *payload, unsigned int length)
{
  String message;
  for (int i = 0; i < length; i++)
    message += (char)payload[i];
  Serial.printf("\nPesan dari [%s]: %s\n", topic, message.c_str());

  if (String(topic) == mqtt_topic_sub)
  {
    StaticJsonDocument<200> doc;
    if (deserializeJson(doc, message))
    {
      Serial.println("Gagal parsing JSON!");
      return;
    }

    int relay1State = doc["relay1"];
    int relay2State = doc["relay2"];

    digitalWrite(RELAYPIN1, relay1State ? HIGH : LOW);
    digitalWrite(RELAYPIN2, relay2State ? HIGH : LOW);
    Serial.printf("Relay1: %s | Relay2: %s\n", relay1State ? "ON" : "OFF", relay2State ? "ON" : "OFF");
  }
}

// ==== Reconnect MQTT ====
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Menghubungkan ke MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str()))
    {
      Serial.println("Terhubung!");
      client.subscribe(mqtt_topic_sub);
    }
    else
    {
      Serial.print("Gagal, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// ==== Setup ====
void setup()
{
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // === Setup Modbus ===
  pinMode(MODBUS_EN_PIN, OUTPUT);
  digitalWrite(MODBUS_EN_PIN, LOW);
  Serial2.begin(MODBUS_SERIAL_BAUD, MODBUS_PARITY, MODBUS_RO_PIN, MODBUS_DI_PIN);
  modbus.begin(MODBUS_SLAVE_ID, Serial2);
  modbus.preTransmission(modbusPreTransmission);
  modbus.postTransmission(modbusPostTransmission);
}

// ==== Loop utama ====
void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000)
  { // kirim data setiap 5 detik
    lastMsg = now;

    // ======== BACA DHT11 ========
    float h_dht = dht.readHumidity();
    float t_dht = dht.readTemperature();
    if (isnan(h_dht) || isnan(t_dht))
    {
      Serial.println("Gagal membaca sensor DHT!");
      return;
    }

    // ======== BACA MODBUS RTU ========
    float t_rtu = 0.0, h_rtu = 0.0;
    int result = modbus.readInputRegisters(MODBUS_ADDRESS, MODBUS_QUANTITY);
    if (result == modbus.ku8MBSuccess)
    {
      int rawTemp = modbus.getResponseBuffer(0x00);
      int rawHum = modbus.getResponseBuffer(0x01);
      t_rtu = rawTemp / 10.0;
      h_rtu = rawHum / 10.0;

      Serial.printf("Modbus => Temp: %.1f °C | Hum: %.1f %%\n", t_rtu, h_rtu);
    }
    else
    {
      Serial.println("Gagal membaca data dari Modbus RTU!");
    }

    // ======== KIRIM DATA KE MQTT ========
    StaticJsonDocument<256> doc;
    doc["temperatureRtu"] = t_rtu;
    doc["humidityRtu"] = h_rtu;
    doc["temperatureDht"] = t_dht;
    doc["humidityDht"] = h_dht;

    char payload[256];
    serializeJson(doc, payload);
    client.publish(mqtt_topic_pub, payload);

    Serial.print("Data dikirim ke MQTT: ");
    Serial.println(payload);
    Serial.println("-----------------------------");
  }
}
