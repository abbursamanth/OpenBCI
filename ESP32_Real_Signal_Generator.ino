/*
  ESP32 OpenBCI Compatible Real Signal Generator
  
  This sketch reads real analog signals from ESP32 ADC pins and sends them
  in OpenBCI format that BrainFlow can understand.
  
  - Reads from 8 analog pins (GPIO32-39)
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

// ESP32 ADC pins for 8 channels
const int ADC_PINS[8] = {32, 33, 34, 35, 36, 39, 34, 35}; // GPIO pins with ADC

// Variables
unsigned long lastSampleTime = 0;
byte sampleNumber = 0;

void setup() {
  Serial.begin(115200); // Same baud rate as Cyton
  
  // Set ADC resolution to 12 bits
  analogReadResolution(12);
  
  // Set ADC attenuation for full 3.3V range
  for (int i = 0; i < 8; i++) {
    analogSetAttenuation(ADC_11db); // 0-3.3V range
  }
  
  Serial.println("ESP32 OpenBCI Compatible Signal Generator Started");
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
    // Read real analog value from ESP32 ADC
    int adcValue = analogRead(ADC_PINS[channel]);
    
    // Convert 12-bit ADC (0-4095) to 24-bit value
    // Scale to use full 24-bit range for better resolution
    int32_t scaledValue = map(adcValue, 0, 4095, -8388607, 8388607); // 24-bit signed range
    
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
