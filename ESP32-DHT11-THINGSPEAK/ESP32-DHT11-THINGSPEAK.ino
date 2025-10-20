#include <WiFi.h>
#include "ThingSpeak.h"
#include "DHT.h"

#define DHTPIN 26     // Pin DHT22
#define DHTTYPE DHT11 // DHT11 atau DHT22 (menyesuaikan tipe dht)

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

const char *ssid = "SSID";     // Nama WiFi
const char *pass = "PASSOWRD"; // Password WiFi

unsigned long myChannelNumber = "ID NUMBER"; // ID Channel ThingSpeak
const char *myWriteAPIKey = "API KEY";       // API Key

const float GAMMA = 0.7;
const float RL10 = 50;

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

void setup()
{
  Serial.begin(9600);
  dht.begin();

  WiFi.mode(WIFI_STA);
  connectToWiFi();

  ThingSpeak.begin(client);
}

void loop()
{
  if ((millis() - lastTime) > timerDelay)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      connectToWiFi();
    }

    // Sensor DHT
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    Serial.println("Temp: " + String(temperature, 2) + "°C");
    Serial.println("Humidity: " + String(humidity, 1) + "%");
    Serial.println("---");

    // Mengirim data ke ThingSpeak
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);

    int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if (responseCode == 200)
    {
      Serial.println("Channel update successful.");
    }
    else
    {
      Serial.println("Problem updating channel. HTTP error code: " + String(responseCode));
    }
    lastTime = millis();
  }
}

void connectToWiFi()
{
  Serial.print("Attempting to connect to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(ssid, pass);
    Serial.print(".");
    delay(5000);
  }
  Serial.println("\nConnected to WiFi.");
}
