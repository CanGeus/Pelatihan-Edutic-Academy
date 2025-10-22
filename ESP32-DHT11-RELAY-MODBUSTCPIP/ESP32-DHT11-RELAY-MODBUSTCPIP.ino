#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include "DHT.h"

#define DHTPIN 27
#define DHTTYPE DHT11
#define RELAY1_PIN 16
#define RELAY2_PIN 17
#define TOUCH1_PIN 13
#define TOUCH2_PIN 12
#define TOUCH3_PIN 14

#define TEMPERATURE_ADDRESS 1
#define HUMIDITY_ADDRESS 2
#define TOUCH1_ADDRESS 3
#define TOUCH2_ADDRESS 4
#define TOUCH3_ADDRESS 5
#define RELAY1_ADDRESS 10
#define RELAY2_ADDRESS 11

const char *ssid = "SSID";
const char *pass = "PASSWORD";

// IPAddress local_IP(192, 168, 144, 113);
// IPAddress gateway(192, 168, 144, 80);
// IPAddress subnet(255, 255, 0, 0);

DHT dht(DHTPIN, DHTTYPE);
ModbusIP mb;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("MODBUS TCP OVER WIFI ESP32"));

  // WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  dht.begin();

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);

  mb.server();

  mb.addHreg(TEMPERATURE_ADDRESS);
  mb.addHreg(HUMIDITY_ADDRESS);
  mb.addHreg(TOUCH1_ADDRESS);
  mb.addHreg(TOUCH2_ADDRESS);
  mb.addHreg(TOUCH3_ADDRESS);
  mb.addCoil(RELAY1_ADDRESS);
  mb.addCoil(RELAY2_ADDRESS);

  mb.Coil(RELAY1_ADDRESS, 0);
  mb.Coil(RELAY2_ADDRESS, 0);
}

void loop()
{

  mb.task();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int touch1 = touchRead(TOUCH1_PIN);
  int touch2 = touchRead(TOUCH2_PIN);
  int touch3 = touchRead(TOUCH3_PIN);

  if (touch1 < 40)
  {
    mb.Hreg(TOUCH1_ADDRESS, 1);
  }
  else
  {
    mb.Hreg(TOUCH1_ADDRESS, 0);
  }
  if (touch2 < 40)
  {
    mb.Hreg(TOUCH2_ADDRESS, 1);
  }
  else
  {
    mb.Hreg(TOUCH2_ADDRESS, 0);
  }
  if (touch3 < 40)
  {
    mb.Hreg(TOUCH3_ADDRESS, 1);
  }
  else
  {
    mb.Hreg(TOUCH3_ADDRESS, 0);
  }

  if (!isnan(humidity) && !isnan(temperature))
  {
    mb.Hreg(TEMPERATURE_ADDRESS, temperature * 10);
    mb.Hreg(HUMIDITY_ADDRESS, humidity * 10);
  }

  bool relay1_state = (mb.Coil(RELAY1_ADDRESS) == 1) ? LOW : HIGH;
  bool relay2_state = (mb.Coil(RELAY2_ADDRESS) == 1) ? LOW : HIGH;

  digitalWrite(RELAY1_PIN, relay1_state);
  digitalWrite(RELAY2_PIN, relay2_state);

  delay(100);
}