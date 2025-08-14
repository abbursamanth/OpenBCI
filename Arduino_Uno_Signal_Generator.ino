/*
  Arduino Uno OpenBCI Compatible Signal Generator
  
  This sketch mimics a Cyton board for BrainFlow compatibility.
  Responds to BrainFlow commands and sends proper data packets.
  
  - Reads from 6 analog pins (A0-A5)
  - Responds to BrainFlow initialization commands
  - Sends OpenBCI binary packets (33 bytes)
  - Sample rate: 250 Hz (standard OpenBCI rate)
  - Compatible with BrainFlow CYTON_BOARD
*/

// OpenBCI packet format constants
const int PACKET_SIZE = 33;
const byte START_BYTE = 0xA0;
const byte END_BYTE = 0xC0;

// Timing for 250Hz sampling (standard OpenBCI rate)
const unsigned long SAMPLE_INTERVAL_US = 4000; // 4000 microseconds = 250Hz

// Arduino Uno analog pins
const int ANALOG_PINS[6] = {A0, A1, A2, A3, A4, A5};

// Variables
unsigned long lastSampleTime = 0;
byte sampleNumber = 0;
bool streamingData = false;
String inputCommand = "";

void setup() {
  Serial.begin(115200); // Standard OpenBCI baud rate
  
  // Set analog reference to default (5V)
  analogReference(DEFAULT);
  
  // Send OpenBCI startup message that BrainFlow expects
  delay(500);
  Serial.println("OpenBCI V3 8-Bit Board");
  Serial.println("On Board ADS1299 Device ID: 0x3E");
  Serial.println("LIS3DH Device ID: 0x33");
  Serial.println("$$$");
  
  delay(1000);
}

void loop() {
  // Handle incoming commands from BrainFlow
  handleSerialCommands();
  
  // Send data packets if streaming is enabled
  if (streamingData) {
    unsigned long currentTime = micros();
    
    if (currentTime - lastSampleTime >= SAMPLE_INTERVAL_US) {
      lastSampleTime = currentTime;
      sendOpenBCIPacket();
      sampleNumber++;
    }
  }
}

void handleSerialCommands() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    if (inChar == '\n' || inChar == '\r') {
      processCommand(inputCommand);
      inputCommand = "";
    } else {
      inputCommand += inChar;
    }
  }
}

void processCommand(String command) {
  command.trim();
  
  if (command == "v") {
    // Version query
    Serial.println("v3.1.1$$$");
  }
  else if (command == "b") {
    // Start streaming
    streamingData = true;
    Serial.println("Stream started$$$");
  }
  else if (command == "s") {
    // Stop streaming
    streamingData = false;
    Serial.println("Stream stopped$$$");
  }
  else if (command == "?") {
    // Help/status
    Serial.println("OpenBCI V3 8-Bit Board$$$");
  }
  else if (command.startsWith("x")) {
    // Channel settings - just acknowledge
    Serial.println("Success: Channel set$$$");
  }
  else if (command == "d") {
    // Default channel settings
    Serial.println("Success: default channel settings$$$");
  }
  else {
    // Unknown command - just acknowledge
    Serial.println("Success$$$");
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
    
    // Add LARGE demo signals for easy identification and testing
    if (channel == 0) {
      // Channel 0: Large sine wave (1 Hz)
      scaledValue = (long)(sin(millis() * 0.001) * 2000000);
    } else if (channel == 1) {
      // Channel 1: Large sine wave (0.5 Hz) 
      scaledValue = (long)(sin(millis() * 0.0005) * 1500000);
    } else if (channel == 2) {
      // Channel 2: Square wave for testing
      scaledValue = (millis() % 1000 < 500) ? 1000000 : -1000000;
    } else if (channel == 3) {
      // Channel 3: Sawtooth wave
      scaledValue = ((millis() % 2000) - 1000) * 1000;
    } else {
      // Other channels: Use real analog + small sine wave
      scaledValue += (long)(sin(millis() * 0.002 + channel) * 500000);
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
