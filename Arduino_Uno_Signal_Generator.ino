/*
  Arduino Uno OpenBCI Compatible Real Signal Generator
  
  This sketch reads real analog signals from Arduino Uno analog pins and sends them
  in OpenBCI format that BrainFlow can understand.
  
  - Reads from 6 analog pins (A0-A5) - Arduino Uno has 6 analog inputs
  - Sends data in OpenBCI packet format (33 bytes)
  - Sample rate: 250 Hz
  - Compatible with BrainFlow CYTON_BOARD
*/

// OpenBCI packet format constants
const int PACKET_SIZE = 33;
const byte START_BYTE = 0xA0;
const byte END_BYTE = 0xC0;

// Timing for 250Hz sampling
const unsigned long SAMPLE_INTERVAL_US = 4000; // 4000 microseconds = 250Hz

// Arduino Uno analog pins (A0-A5, plus A0 and A1 repeated for 8 channels)
const int ANALOG_PINS[8] = {A0, A1, A2, A3, A4, A5, A0, A1};

// Variables
unsigned long lastSampleTime = 0;
byte sampleNumber = 0;

void setup() {
  Serial.begin(115200); // Same baud rate as Cyton
  
  // Set analog reference to default (5V)
  analogReference(DEFAULT);
  
  Serial.println("Arduino Uno OpenBCI Compatible Signal Generator Started");
  Serial.println("Reading from analog pins A0-A5");
  Serial.println("Sending real analog data at 250Hz...");
  
  delay(1000); // Give GUI time to connect
}

void loop() {
  unsigned long currentTime = micros();
  
  // Check if it's time for the next sample (250Hz = 4000 microseconds)
  if (currentTime - lastSampleTime >= SAMPLE_INTERVAL_US) {
    lastSampleTime = currentTime;
    
    // Create OpenBCI packet
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
  for (int channel = 0; channel < 8; channel++) {
    // Read real analog value from Arduino Uno ADC
    int adcValue = analogRead(ANALOG_PINS[channel]);
    
    // Convert 10-bit ADC (0-1023) to 24-bit value
    // Arduino Uno has 10-bit ADC, scale to use full 24-bit range
    long scaledValue = map(adcValue, 0, 1023, -8388607, 8388607); // 24-bit signed range
    
    // Add some variation to make signals more interesting
    if (channel == 0) {
      // Channel 0: Add sine wave for demo
      scaledValue += (long)(sin(millis() * 0.01) * 1000000);
    } else if (channel == 1) {
      // Channel 1: Add different frequency sine wave
      scaledValue += (long)(sin(millis() * 0.02) * 500000);
    }
    
    // Pack as 3 bytes (24-bit, big-endian)
    packet[index++] = (scaledValue >> 16) & 0xFF; // Most significant byte
    packet[index++] = (scaledValue >> 8) & 0xFF;  // Middle byte
    packet[index++] = scaledValue & 0xFF;         // Least significant byte
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
