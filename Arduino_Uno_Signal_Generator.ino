/*
  Arduino Uno Simple Signal Generator for OpenBCI GUI
  
  This sketch sends readable analog data that can be processed by the GUI.
  Simplified approach - sends CSV-like data for easier debugging.
  
  - Reads from 6 analog pins (A0-A5)
  - Sends readable data format
  - Sample rate: ~125 Hz (every 8ms)
*/

// Timing for sampling
const unsigned long SAMPLE_INTERVAL_MS = 8; // 8ms = ~125Hz (more realistic for serial)

// Arduino Uno analog pins
const int ANALOG_PINS[6] = {A0, A1, A2, A3, A4, A5};
const int NUM_CHANNELS = 6;

// Variables
unsigned long lastSampleTime = 0;
unsigned int sampleNumber = 0;

void setup() {
  Serial.begin(9600); // Reliable baud rate
  
  // Set analog reference to default (5V)
  analogReference(DEFAULT);
  
  // Send startup message
  Serial.println("Arduino Uno Signal Generator Started");
  Serial.println("Format: Sample,A0,A1,A2,A3,A4,A5");
  Serial.println("Starting data stream...");
  
  delay(2000); // Give time to read messages
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check if it's time for the next sample
  if (currentTime - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = currentTime;
    
    // Send readable data
    sendReadableData();
    
    sampleNumber++;
  }
}

void sendReadableData() {
  // Send sample number
  Serial.print(sampleNumber);
  Serial.print(",");
  
  // Read and send all analog channels
  for (int channel = 0; channel < NUM_CHANNELS; channel++) {
    int adcValue = analogRead(ANALOG_PINS[channel]);
    
    // Convert to voltage (0-5V range)
    float voltage = (adcValue * 5.0) / 1023.0;
    
    // Add some demo signals for testing
    if (channel == 0) {
      // Channel 0: Add sine wave
      voltage += sin(millis() * 0.01) * 0.5 + 2.5; // 0-5V range
    } else if (channel == 1) {
      // Channel 1: Add different frequency
      voltage += sin(millis() * 0.02) * 0.3 + 2.5;
    }
    
    // Send voltage value
    Serial.print(voltage, 3); // 3 decimal places
    
    if (channel < NUM_CHANNELS - 1) {
      Serial.print(",");
    }
  }
  
  Serial.println(); // End line
}
