
#include <ModbusMaster.h>

#define MODBUS_EN_PIN 4          // DE dan RE pin
#define MODBUS_RO_PIN 18         // RO pin
#define MODBUS_DI_PIN 19         // DI pin
#define MODBUS_SERIAL_BAUD 9600  // Baud rate MODBUS
#define MODBUS_PARITY SERIAL_8N1 // parity Modbus

#define MODBUS_SLAVE_ID 1     // slave ID modbus
#define MODBUS_ADDRESS 0x0001 // alamat modbus yang diambil datanya
#define MODBUS_QUANTITY 2     // jumlah data yang diambil

ModbusMaster modbus;

// Pin 4 made high for Modbus transmision mode
void modbusPreTransmission()
{
  delay(500);
  digitalWrite(MODBUS_EN_PIN, HIGH);
}
// Pin 4 made low for Modbus receive mode
void modbusPostTransmission()
{
  digitalWrite(MODBUS_EN_PIN, LOW);
  delay(500);
}

void setup()
{
  Serial.begin(115200);
  pinMode(MODBUS_EN_PIN, OUTPUT);
  digitalWrite(MODBUS_EN_PIN, LOW);

  Serial2.begin(MODBUS_SERIAL_BAUD, MODBUS_PARITY, MODBUS_RO_PIN, MODBUS_DI_PIN);
  Serial2.setTimeout(1000);
  modbus.begin(MODBUS_SLAVE_ID, Serial2);

  modbus.preTransmission(modbusPreTransmission);
  modbus.postTransmission(modbusPostTransmission);
}

void loop()
{
  int pooling;
  int hasil[2];
  float temperature;
  float humidity;

  pooling = modbus.readInputRegisters(MODBUS_ADDRESS, MODBUS_QUANTITY);
  if (pooling == modbus.ku8MBSuccess)
  {
    Serial.println("Success, Data diterima: ");

    hasil[0] = modbus.getResponseBuffer(0x00);
    hasil[1] = modbus.getResponseBuffer(0x01);

    temperature = hasil[0] / 10.f;
    humidity = hasil[1] / 10.f;

    Serial.println("temperature: " + String(temperature));
    Serial.println("Humidity: " + String(humidity));
    Serial.println();
  }
  else
  {
    Serial.println("GAGAL membaca data");
  }
  delay(1000);
}
esp32_modbus.txt

    Displaying esp32_modbus.txt.