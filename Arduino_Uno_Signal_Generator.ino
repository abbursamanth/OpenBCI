/*
  Arduino Uno OpenBCI Compatible Signal Generator
  
  This sketch sends data in OpenBCI binary format that BrainFlow can understand.
  Back to the working approach - no CSV, just proper OpenBCI packets.
  
  - Reads from 6 analog pins (A0-A5)
  - Sends OpenBCI binary packets (33 bytes)
  - Sample rate: 50 Hz (stable for Arduino)
  - Compatible with BrainFlow CYTON_BOARD
*/

// OpenBCI packet format constants
const int PACKET_SIZE = 33;
const byte START_BYTE = 0xA0;
const byte END_BYTE = 0xC0;

// Timing for 50Hz sampling
const unsigned long SAMPLE_INTERVAL_MS = 20; // 20ms = 50Hz

// Arduino Uno analog pins
const int ANALOG_PINS[6] = {A0, A1, A2, A3, A4, A5};

// Variables
unsigned long lastSampleTime = 0;
byte sampleNumber = 0;

void setup() {
  Serial.begin(115200); // Standard OpenBCI baud rate
  
  // Set analog reference to default (5V)
  analogReference(DEFAULT);
  
  delay(1000); // Give GUI time to connect
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check if it's time for the next sample
  if (currentTime - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = currentTime;
    
    // Send OpenBCI packet
    sendOpenBCIPacket();
    
    sampleNumber++;
  }
}

void sendOpenBCIPacket() {
  byte packet[PACKET_SIZE];
  int index = 0;
  
  // Start byte
  packet[index++] = START_BYTE;
  
  // Sample number (8-bit counter)
  packet[index++] = sampleNumber;
  
  // 8 EEG channels (3 bytes each, 24-bit)
  // Use 6 real channels + 2 repeated for OpenBCI format
  for (int channel = 0; channel < 8; channel++) {
    int adcValue;
    
    if (channel < 6) {
      // Real analog channels A0-A5
      adcValue = analogRead(ANALOG_PINS[channel]);
    } else {
      // Repeat A0 and A1 for channels 6 and 7
      adcValue = analogRead(ANALOG_PINS[channel - 6]);
    }
    
    // Convert 10-bit ADC (0-1023) to 24-bit signed value
    // Center around 0 and scale appropriately
    long scaledValue = (long)(adcValue - 512) * 16384; // Scale to use 24-bit range
    
    // Add demo signals for easy identification
    if (channel == 0) {
      scaledValue += (long)(sin(millis() * 0.01) * 100000);
    } else if (channel == 1) {
      scaledValue += (long)(sin(millis() * 0.02) * 50000);
    }
    
    // Pack as 3 bytes (24-bit, big-endian)
    packet[index++] = (scaledValue >> 16) & 0xFF; // MSB
    packet[index++] = (scaledValue >> 8) & 0xFF;  // Middle
    packet[index++] = scaledValue & 0xFF;         // LSB
  }
  
  // 3 Accelerometer channels (2 bytes each, 16-bit) - dummy data
  for (int i = 0; i < 3; i++) {
    packet[index++] = 0x00; // High byte
    packet[index++] = 0x00; // Low byte
  }
  
  // End byte
  packet[index++] = END_BYTE;
  
  // Send packet over serial
  Serial.write(packet, PACKET_SIZE);
}
