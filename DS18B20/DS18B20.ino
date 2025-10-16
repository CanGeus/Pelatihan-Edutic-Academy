#include <OneWire.h>
#include <DallasTemperature.h>

const int oneWireBus = 4;

OneWire oneWire(oneWireBus);

DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  sensors.begin();
}

void loop() {
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.println("Suhu : ");
  Serial.print(temperatureC);
  Serial.println(" ºC");
  Serial.print(temperatureF);
  Serial.println(" ºF");
  Serial.println("--------------------");
  delay(5000);
}
