// Include Libraries 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

// LCD I2C Setup 
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DHT Sensor Setup 
#define DHTPIN 32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float humidity, temperature;

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
bool showDHT = true;
unsigned long lastTouchTime = 0;
const int debounceDelay = 1000;


// Function Prototypes 
void readTouch();
void readDHT();
void readUltrasonic();
void displayLCD(String line1, String line2);

// Setup 
void setup() {
  Serial.begin(115200);

  // Initialize sensors
  dht.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize LCD
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

  if (showDHT) {
    readDHT();
    displayLCD("Hum: " + String(humidity, 1) + "%", "Temp: " + String(temperature, 1) + "C");
  } else {
    readUltrasonic();
    String text;
    if (distanceCm <= 10){
      text = "     Bahaya";
    } else if (distanceCm > 10 && distanceCm <= 20) {
      text = "Bahaya Mendekat";
    } else if (distanceCm > 20 && distanceCm <= 30) {
      text = " Bahaya Menjauh";
    } else if (distanceCm > 30){
      text = "      Aman";
    }
    displayLCD("Jarak: " + String(distanceCm, 1) + " cm", text);
  }

  delay(1000);
}

// Read Touch Sensor 
void readTouch() {
  touchValue = touchRead(TOUCH_PIN);
  Serial.print("Touch Value: ");
  Serial.println(touchValue);

  unsigned long currentTime = millis();

  if (touchValue < threshold && (currentTime - lastTouchTime > debounceDelay)) {
    showDHT = !showDHT;
    lastTouchTime = currentTime;
    Serial.println("Tampilan berubah!");
  }
}

// Read DHT Sensor 
void readDHT() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%  |  Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
}

// Read Ultrasonic Sensor =
void readUltrasonic() {
  // Trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read echo time
  duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate distance
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
