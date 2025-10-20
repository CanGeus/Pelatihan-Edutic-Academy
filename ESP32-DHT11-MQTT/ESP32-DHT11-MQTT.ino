#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// ==== Konfigurasi DHT ====
#define DHTPIN 25
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ==== Konfigurasi WiFi ====
const char *ssid = "SSID";
const char *password = "PASSWORD";

// ==== Konfigurasi MQTT ====
const char *mqtt_server = "broker.emqx.io"; // broker MQTT (MQTTX public broker)
const int mqtt_port = 1883;                 // port non-SSL
const char *mqtt_topic_temp = "esp32/dht/temperature";
const char *mqtt_topic_hum = "esp32/dht/humidity";

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

    char tempStr[8];
    char humStr[8];
    dtostrf(t, 1, 2, tempStr);
    dtostrf(h, 1, 2, humStr);

    // Publish ke MQTT broker
    client.publish(mqtt_topic_temp, tempStr);
    client.publish(mqtt_topic_hum, humStr);

    // Debug di Serial Monitor
    Serial.print("Suhu: ");
    Serial.print(tempStr);
    Serial.print(" °C | Kelembapan: ");
    Serial.print(humStr);
    Serial.println(" %");
  }
}
