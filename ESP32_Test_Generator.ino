/*
 * ESP32 Simple Test Signal Generator for OpenBCI GUI
 * This sketch generates simple test signals to verify communication
 * 
 * Upload this first to test if the serial communication is working
 * Once this works, you can switch to the full EEG generator
 */

// OpenBCI packet format
const int PACKET_SIZE = 33; // bytes
const unsigned long SAMPLE_PERIOD = 4000; // 4ms = 250Hz
byte packet[PACKET_SIZE];
byte sampleCounter = 0;
unsigned long lastSampleTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Test Signal Generator Started");
  Serial.println("Sending test data at 250 Hz");
  delay(1000);
}

void loop() {
  unsigned long currentTime = micros();
  
  if (currentTime - lastSampleTime >= SAMPLE_PERIOD) {
    sendTestPacket();
    lastSampleTime = currentTime;
  }
}

void sendTestPacket() {
  // OpenBCI packet format:
  // Byte 0: Start byte (0xA0)
  // Byte 1: Sample number (0-255)
  // Bytes 2-25: Channel data (8 channels × 3 bytes each)
  // Bytes 26-31: Auxiliary data
  // Byte 32: End byte (0xC0)
  
  packet[0] = 0xA0; // Start byte
  packet[1] = sampleCounter; // Sample number
  
  // Generate simple test signals for 8 channels
  for (int ch = 0; ch < 8; ch++) {
    int32_t channelValue;
    
    // Generate different test patterns for each channel
    switch (ch) {
      case 0: // Sine wave 1Hz
        channelValue = (int32_t)(100000 * sin(2 * PI * 1.0 * millis() / 1000.0));
        break;
      case 1: // Sine wave 2Hz
        channelValue = (int32_t)(100000 * sin(2 * PI * 2.0 * millis() / 1000.0));
        break;
      case 2: // Square wave 0.5Hz
        channelValue = (millis() % 2000 < 1000) ? 100000 : -100000;
        break;
      case 3: // Sawtooth wave
        channelValue = (int32_t)((millis() % 1000) * 200 - 100000);
        break;
      case 4: // Random noise
        channelValue = random(-50000, 50000);
        break;
      case 5: // DC offset
        channelValue = 50000;
        break;
      case 6: // Inverted sine 1Hz
        channelValue = (int32_t)(-100000 * sin(2 * PI * 1.0 * millis() / 1000.0));
        break;
      case 7: // Triangle wave
        {
          int period = millis() % 2000;
          if (period < 1000) {
            channelValue = (int32_t)(period * 200 - 100000);
          } else {
            channelValue = (int32_t)(300000 - period * 200);
          }
        }
        break;
    }
    
    // Clamp to 24-bit signed range
    if (channelValue > 8388607) channelValue = 8388607;
    if (channelValue < -8388608) channelValue = -8388608;
    
    // Pack as 24-bit signed integer (big-endian)
    int baseIndex = 2 + ch * 3;
    packet[baseIndex] = (channelValue >> 16) & 0xFF;
    packet[baseIndex + 1] = (channelValue >> 8) & 0xFF;
    packet[baseIndex + 2] = channelValue & 0xFF;
  }
  
  // Auxiliary data (set to 0)
  for (int i = 26; i < 32; i++) {
    packet[i] = 0x00;
  }
  
  packet[32] = 0xC0; // End byte
  
  // Send packet
  Serial.write(packet, PACKET_SIZE);
  
  // Increment sample counter
  sampleCounter++;
  
  // Debug output every 250 samples (1 second)
  if (sampleCounter == 0) {
    Serial.print("Sent 256 packets, time: ");
    Serial.println(millis());
  }
}
