// Include Libraries 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// LCD I2C Setup 
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Ultrasonic Setup
#define TRIG_PIN 25
#define ECHO_PIN 26
#define SOUND_SPEED 0.034 // cm per microsecond
#define CM_TO_INCH 0.393701

long duration;
float distanceCm, distanceInch;

// Touch Sensor Setup
#define TOUCH_PIN 13
int touchValue = 0;
int threshold = 50;
bool showTemp = true;
unsigned long lastTouchTime = 0;
const int debounceDelay = 1000;

// DS18B20 Setup
const int oneWireBus = 14;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
float temperatureC;

// Relay Setup
#define RELAY1 32
#define RELAY2 33

// Function Prototypes
void temp();
void readTouch();
void readUltrasonic();
void displayLCD(String line1, String line2);
void relayControl(int pin, int state);

// Global variable
String message;

// Setup 
void setup() {
  Serial.begin(115200);

  sensors.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 Starting...");
  delay(1500);
  lcd.clear();
}

// Main Loop 
void loop() {
  readTouch();
  readUltrasonic();
  temp();

  // Logika suhu
  if (temperatureC < 24) {
    message = "Suhu Air Dingin";
  } else if (temperatureC >= 24 && temperatureC <= 30) {
    message = "Suhu Air Normal";
  } else if (temperatureC > 30) {
    message = "Suhu Air Panas";
  } else {
    message = "Sensor Error";
  }

  // Logika relay (ubah sesuai kebutuhan alat)
  if (distanceCm <= 5) {
    relayControl(RELAY1, LOW);
  } else if (distanceCm > 5 && distanceCm <= 10) {
    relayControl(RELAY1, HIGH);
  } else {
    relayControl(RELAY1, HIGH);
  }

  // Tampilan LCD berganti antara suhu dan jarak
  if (showTemp) {
    displayLCD("Temp: " + String(temperatureC, 1) + " C", message);
  } else {
    displayLCD("Jarak: " + String(distanceCm, 1) + " cm", "Relay " + String(digitalRead(RELAY1) == HIGH ? "ON" : "OFF"));
  }

  
  Serial.println("--------------------");
  delay(1000);
}

// Read Ultrasonic Sensor
void readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
  distanceInch = distanceCm * CM_TO_INCH;

  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.print(" cm | ");
  Serial.print(distanceInch);
  Serial.println(" inch");
}

// LCD Display 
void displayLCD(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// Read DS18B20
void temp() {
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);

  Serial.print("Suhu: ");
  Serial.print(temperatureC);
  Serial.println(" °C");
}

// Relay Control
void relayControl(int pin, int state) {
  digitalWrite(pin, state);
  Serial.print("Relay on pin ");
  Serial.print(pin);
  Serial.print(" is now ");
  Serial.println(state == HIGH ? "ON" : "OFF");
}

// Read Touch Sensor 
void readTouch() {
  touchValue = touchRead(TOUCH_PIN);
  Serial.print("Touch Value: ");
  Serial.println(touchValue);

  unsigned long currentTime = millis();
  if (touchValue < threshold && (currentTime - lastTouchTime > debounceDelay)) {
    showTemp = !showTemp;
    lastTouchTime = currentTime;
    Serial.println("Tampilan berubah!");
  }
}
