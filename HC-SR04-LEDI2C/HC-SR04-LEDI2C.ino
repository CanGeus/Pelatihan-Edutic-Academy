// Define LCDI2C
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);

// Define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

// Define hc-sr04
const int trigPin = 25;
const int echoPin = 26;
long duration;
int distanceCm;
int distanceInch;

void setup() {

  // Start Serial
  Serial.begin(115200);

  // Start HC-SR04
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Start LCDI2C
  lcd.init();

  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("ESP32 Start");
  delay(1000);
  lcd.clear();

}

void loop() {

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  // Convert to inches
  distanceInch = distanceCm * CM_TO_INCH;

  led();
}

void led(){
  lcd.setCursor(0, 0);
  lcd.print(distanceCm);
  lcd.print(" Cm");
  lcd.setCursor(0, 1);
  lcd.print(distanceInch);
  lcd.print(" Inch");
  delay(1000);
  lcd.clear();
}
