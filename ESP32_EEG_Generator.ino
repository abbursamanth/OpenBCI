/*
 * ESP32 EEG Signal Generator for OpenBCI GUI
 * This sketch generates real analog signals from ESP32 ADC and formats them
 * to be compatible with OpenBCI Cyton board format
 * 
 * Connections:
 * - Connect analog sensors to GPIO32, GPIO33, GPIO34, GPIO35 (ADC1 channels)
 * - These will be treated as EEG channels 1-4
 * - You can connect potentiometers, EMG sensors, or any analog signals
 * 
 * Serial Output: 115200 baud, compatible with OpenBCI format
 */

// ADC pins for analog inputs (use ADC1 channels to avoid WiFi conflicts)
const int analogPins[] = {32, 33, 34, 35, 36, 39}; // 6 channels
const int numChannels = 6;

// OpenBCI packet format
const int SAMPLE_RATE = 250; // Hz
const int PACKET_SIZE = 33; // bytes
const unsigned long SAMPLE_PERIOD = 1000000 / SAMPLE_RATE; // microseconds

// Packet structure
byte packet[PACKET_SIZE];
byte sampleCounter = 0;
unsigned long lastSampleTime = 0;

// Calibration values for ADC (ESP32 ADC is 12-bit: 0-4095)
const float ADC_MAX = 4095.0;
const float ADC_VOLTAGE = 3.3; // ESP32 ADC reference voltage

void setup() {
  Serial.begin(115200);
  
  // Initialize ADC pins
  for (int i = 0; i < numChannels; i++) {
    pinMode(analogPins[i], INPUT);
  }
  
  // Set ADC resolution to 12 bits
  analogReadResolution(12);
  
  Serial.println("ESP32 EEG Signal Generator Started");
  Serial.println("Sending OpenBCI-compatible data at 250 Hz");
  delay(1000);
}

void loop() {
  unsigned long currentTime = micros();
  
  if (currentTime - lastSampleTime >= SAMPLE_PERIOD) {
    sendDataPacket();
    lastSampleTime = currentTime;
  }
}

void sendDataPacket() {
  // OpenBCI packet format:
  // Byte 0: Start byte (0xA0)
  // Byte 1: Sample number (0-255)
  // Bytes 2-25: Channel data (8 channels × 3 bytes each, 24-bit signed)
  // Bytes 26-31: Auxiliary data (3 × 2 bytes, accelerometer)
  // Byte 32: End byte (0xC0)
  
  packet[0] = 0xA0; // Start byte
  packet[1] = sampleCounter; // Sample number
  
  // Read and pack 8 channels (6 real + 2 virtual)
  for (int ch = 0; ch < 8; ch++) {
    int32_t channelValue;
    
    if (ch < numChannels) {
      // Read real analog value
      int adcValue = analogRead(analogPins[ch]);
      
      // Convert to 24-bit signed value similar to OpenBCI range
      // OpenBCI range is approximately ±187.5 μV, we'll scale our ADC accordingly
      channelValue = map(adcValue, 0, 4095, -8388608, 8388607); // 24-bit signed range
    } else {
      // Virtual channels - can be set to 0 or generate derived signals
      channelValue = 0;
    }
    
    // Pack as 24-bit signed integer (big-endian)
    int baseIndex = 2 + ch * 3;
    packet[baseIndex] = (channelValue >> 16) & 0xFF;
    packet[baseIndex + 1] = (channelValue >> 8) & 0xFF;
    packet[baseIndex + 2] = channelValue & 0xFF;
  }
  
  // Auxiliary data (accelerometer) - set to 0 for now
  for (int i = 26; i < 32; i++) {
    packet[i] = 0x00;
  }
  
  packet[32] = 0xC0; // End byte
  
  // Send packet
  Serial.write(packet, PACKET_SIZE);
  
  // Increment sample counter
  sampleCounter++;
}

/*
 * Alternative function for test signals
 * Uncomment this and modify loop() to use generateTestSignals() instead of reading ADC
 */
/*
void generateTestSignals() {
  static float phase = 0;
  
  packet[0] = 0xA0; // Start byte
  packet[1] = sampleCounter; // Sample number
  
  // Generate test signals
  for (int ch = 0; ch < 8; ch++) {
    int32_t channelValue;
    
    // Generate different frequency sine waves for each channel
    float frequency = 1.0 + ch * 2.0; // 1Hz, 3Hz, 5Hz, etc.
    float amplitude = 1000000; // Adjust amplitude as needed
    channelValue = (int32_t)(amplitude * sin(2 * PI * frequency * phase));
    
    // Clamp to 24-bit signed range
    if (channelValue > 8388607) channelValue = 8388607;
    if (channelValue < -8388608) channelValue = -8388608;
    
    // Pack as 24-bit signed integer (big-endian)
    int baseIndex = 2 + ch * 3;
    packet[baseIndex] = (channelValue >> 16) & 0xFF;
    packet[baseIndex + 1] = (channelValue >> 8) & 0xFF;
    packet[baseIndex + 2] = channelValue & 0xFF;
  }
  
  // Auxiliary data (accelerometer) - set to 0
  for (int i = 26; i < 32; i++) {
    packet[i] = 0x00;
  }
  
  packet[32] = 0xC0; // End byte
  
  // Send packet
  Serial.write(packet, PACKET_SIZE);
  
  // Update phase for next sample
  phase += 1.0 / SAMPLE_RATE;
  if (phase > 1.0) phase -= 1.0;
  
  // Increment sample counter
  sampleCounter++;
}
*/
