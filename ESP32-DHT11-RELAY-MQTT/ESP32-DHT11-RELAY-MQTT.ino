#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

// ==== Konfigurasi DHT ====
#define DHTPIN 25
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// === Konfigurasi Relay ===
#define RELAYPIN1 32
#define RELAYPIN2 33

// ==== Konfigurasi WiFi ====
const char *ssid = "SSID";
const char *password = "PASSWORD";

// ==== Konfigurasi MQTT ====
const char *mqtt_server = "broker.emqx.io"; // Public broker MQTT (untuk MQTTX)
const int mqtt_port = 1883;                 // Port tanpa SSL
const char *mqtt_topic_pub = "topic/dht";   // Topik publish data
const char *mqtt_topic_sub = "topic/relay"; // Topik untuk kontrol relay

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

// ==== Koneksi WiFi ====
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

// ==== Callback (pesan diterima dari MQTT) ====
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("\nPesan diterima [");
  Serial.print(topic);
  Serial.print("] ");

  String message;
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Jika pesan datang dari topik relay
  if (String(topic) == mqtt_topic_sub)
  {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
      Serial.print("Gagal parsing JSON: ");
      Serial.println(error.c_str());
      return;
    }

    int relay1State = doc["relay1"];
    int relay2State = doc["relay2"];

    digitalWrite(RELAYPIN1, relay1State ? HIGH : LOW);
    digitalWrite(RELAYPIN2, relay2State ? HIGH : LOW);

    Serial.print("Relay 1: ");
    Serial.println(relay1State ? "ON" : "OFF");
    Serial.print("Relay 2: ");
    Serial.println(relay2State ? "ON" : "OFF");
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
      client.subscribe(mqtt_topic_sub); // Subscribe ke topik kontrol relay
      Serial.print("Subscribed ke topik: ");
      Serial.println(mqtt_topic_sub);
    }
    else
    {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
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
}

// ==== Loop utama ====
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000)
  { // kirim data setiap 5 detik
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t))
    {
      Serial.println("Gagal membaca data dari sensor DHT!");
      return;
    }

    // Buat payload JSON untuk publish
    StaticJsonDocument<200> doc;
    doc["temperature"] = t;
    doc["humidity"] = h;
    doc["device"] = "CanGeus";

    char buffer[200];
    serializeJson(doc, buffer);

    client.publish(mqtt_topic_pub, buffer);
    Serial.print("Data terkirim ke MQTT: ");
    Serial.println(buffer);
  }
}
