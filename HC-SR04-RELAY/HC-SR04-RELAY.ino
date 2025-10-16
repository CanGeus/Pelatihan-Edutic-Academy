/**
 * @file ultrasonic_relay_control.ino
 * @author CanGeus
 * @brief Controls two relays based on the distance measured by an HC-SR04 ultrasonic sensor.
 * @version 0.1
 * @date 2025-10-16
 *
 * @details
 * This sketch measures distance using an HC-SR04 sensor.
 * - If the distance is 5 cm or less, Relay 2 is activated.
 * - If the distance is between 5 cm and 10 cm, Relay 1 is activated.
 * - If the distance is greater than 10 cm, both relays are off.
 *
 * Hardware Connections:
 * - HC-SR04 Trig Pin -> GPIO 25
 * - HC-SR04 Echo Pin -> GPIO 26
 * - Relay 1 IN -> GPIO 32
 * - Relay 2 IN -> GPIO 33
 */

// Define hardware pins
const int TRIG_PIN = 25;
const int ECHO_PIN = 26;
const int RELAY_1_PIN = 32;
const int RELAY_2_PIN = 33;

// Define physical constants
const float SOUND_SPEED_CM_PER_US = 0.034; // Speed of sound in cm/microsecond

// Define distance thresholds
const int MIN_DISTANCE_CM = 5;
const int MAX_DISTANCE_CM = 10;


void setup() {
  Serial.begin(115200);

  // Initialize sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize relay pins and set their default state to OFF
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  digitalWrite(RELAY_1_PIN, LOW);
  digitalWrite(RELAY_2_PIN, LOW);
}

void loop() {
  // --- 1. Trigger the sensor to send a sound wave ---
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // --- 2. Measure the travel time of the sound wave ---
  // pulseIn returns the duration in microseconds for the echo pin to go HIGH.
  long duration_us = pulseIn(ECHO_PIN, HIGH);

  // --- 3. Calculate the distance ---
  // The sound wave travels to the object and back, so we divide by 2.
  float distanceCm = duration_us * SOUND_SPEED_CM_PER_US / 2.0;

  // --- 4. Print the measured distance ---
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");

  // --- 5. Control relays based on the distance ---
  // The original code had a logical error where a condition could never be true.
  // This has been corrected to check for a range between 5 and 10 cm.
  if (distanceCm <= MIN_DISTANCE_CM && distanceCm > 0) {
    // Object is very close
    setRelayState(RELAY_1_PIN, LOW);  // Turn Relay 1 OFF
    setRelayState(RELAY_2_PIN, HIGH); // Turn Relay 2 ON
  } else if (distanceCm > MIN_DISTANCE_CM && distanceCm <= MAX_DISTANCE_CM) {
    // Object is at a medium distance
    setRelayState(RELAY_1_PIN, HIGH); // Turn Relay 1 ON
    setRelayState(RELAY_2_PIN, LOW);  // Turn Relay 2 OFF
  } else {
    // Object is far away or sensor read error (distance is 0)
    setRelayState(RELAY_1_PIN, LOW);  // Turn Relay 1 OFF
    setRelayState(RELAY_2_PIN, LOW);  // Turn Relay 2 OFF
  }

  Serial.println("-----------------------");
  delay(1000); // Wait for a second before the next reading
}

/**
 * @brief Sets the state of a relay and prints its status to the Serial Monitor.
 *
 * @param relayPin The GPIO pin number connected to the relay.
 * @param state    The desired state (HIGH for ON, LOW for OFF).
 */
void setRelayState(int relayPin, int state) {
  digitalWrite(relayPin, state);
  Serial.print("Relay on pin ");
  Serial.print(relayPin);
  Serial.print(" is now ");
  Serial.println(state == HIGH ? "ON" : "OFF");
}
