/*
 * Hall Sensor Parking Simulator
 * Monitors parking slots using Hall effect sensors.
 * Displays occupancy status in real-time via Serial.
 */

#define NUM_SLOTS 6
const int hallPins[NUM_SLOTS] = {2, 3, 4, 5, 6, 7};  // Hall sensor inputs
const int ledPins[NUM_SLOTS]  = {8, 9, 10, 11, 12, 13}; // Occupancy LEDs

bool occupied[NUM_SLOTS] = {false};
int availableSpots = NUM_SLOTS;

void setup() {
  Serial.begin(9600);
  Serial.println("=== Hall Sensor Parking System ===");
  Serial.print("Total slots: ");
  Serial.println(NUM_SLOTS);
  
  for (int i = 0; i < NUM_SLOTS; i++) {
    pinMode(hallPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
}

void loop() {
  availableSpots = 0;
  
  for (int i = 0; i < NUM_SLOTS; i++) {
    int sensorState = digitalRead(hallPins[i]);
    // Hall sensor triggered = vehicle present = slot occupied
    occupied[i] = (sensorState == LOW);
    
    digitalWrite(ledPins[i], occupied[i] ? HIGH : LOW);
    
    if (!occupied[i]) {
      availableSpots++;
    }
  }
  
  printStatus();
  delay(500);
}

void printStatus() {
  Serial.print("Slots: ");
  for (int i = 0; i < NUM_SLOTS; i++) {
    Serial.print("[");
    Serial.print(i + 1);
    Serial.print(":");
    Serial.print(occupied[i] ? "X" : "O");
    Serial.print("] ");
  }
  Serial.print(" | Available: ");
  Serial.print(availableSpots);
  Serial.print("/");
  Serial.println(NUM_SLOTS);
}
