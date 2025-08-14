// ESP32 Custom Board Implementation
// Direct serial communication without BrainFlow dependencies
// This implementation reads raw serial data from ESP32 and processes it directly

import org.apache.commons.lang3.tuple.Pair;
import org.apache.commons.lang3.tuple.ImmutablePair;

class BoardESP32 extends Board implements AccelerometerCapableBoard, AnalogCapableBoard, DigitalCapableBoard, ImpedanceSettingsBoard {

    private String serialPort;
    private String wifiIP;
    private String bluetoothDevice;
    private String connectionType = "Serial";
    private int[] emptyArray = new int[0]; // Empty array for unsupported features
    
    // Serial communication
    private Serial serial_ESP32;
    private boolean isStreaming = false;
    private byte[] packetBuffer = new byte[33]; // OpenBCI packet size
    private int bufferIndex = 0;
    private boolean lookingForStart = true;
    
    // Data processing
    private final int NUM_CHANNELS = 8;
    private final int SAMPLE_RATE = 250;
    private double[][] dataBuffer;
    private int bufferSize = SAMPLE_RATE; // 1 second of data
    private int writeIndex = 0;
    private long lastSampleTime = 0;
    
    // Channel configuration
    private boolean[] channelActive = new boolean[NUM_CHANNELS];

    // Constructor
    public BoardESP32() {
        super();
        initializeChannels();
    }

    public BoardESP32(String connectionInfo) {
        super();
        this.serialPort = connectionInfo;
        initializeChannels();
    }

    public BoardESP32(String connectionInfo, String connType) {
        super();
        this.connectionType = connType;
        
        if (connType.equals("Serial")) {
            this.serialPort = connectionInfo;
        } else if (connType.equals("WiFi")) {
            this.wifiIP = connectionInfo;
        } else if (connType.equals("Bluetooth")) {
            this.bluetoothDevice = connectionInfo;
        }
        initializeChannels();
    }
    
    private void initializeChannels() {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            channelActive[i] = true;
        }
        
        // Initialize data buffer
        dataBuffer = new double[NUM_CHANNELS][bufferSize];
        for (int i = 0; i < NUM_CHANNELS; i++) {
            for (int j = 0; j < bufferSize; j++) {
                dataBuffer[i][j] = 0.0;
            }
        }
    }

    
    // --- Core Board Interface Methods ---
    
    @Override
    public boolean initializeInternal() {
        try {
            println("BoardESP32: Initializing with " + connectionType + " connection");
            
            if (connectionType.equals("Serial")) {
                println("BoardESP32: Connecting to serial port: " + serialPort);
                serial_ESP32 = new Serial(ourApplet, serialPort, 115200);
                serial_ESP32.clear();
                println("BoardESP32: Serial connection established");
            } else {
                outputError("BoardESP32: Only Serial connection is currently supported");
                return false;
            }
            
            isStreaming = false;
            bufferIndex = 0;
            lookingForStart = true;
            writeIndex = 0;
            lastSampleTime = millis();
            
            println("BoardESP32: Initialization successful");
            return true;

        } catch (Exception e) {
            outputError("BoardESP32 ERROR: " + e.getMessage() + " when initializing. Check connection and port.");
            e.printStackTrace();
            return false;
        }
    }
    
    @Override
    public void uninitializeInternal() {
        try {
            if (serial_ESP32 != null) {
                serial_ESP32.stop();
                serial_ESP32 = null;
                println("BoardESP32: Serial connection closed");
            }
            isStreaming = false;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    @Override
    public boolean startStreaming() {
        try {
            if (serial_ESP32 != null) {
                isStreaming = true;
                println("BoardESP32: Started streaming");
                return true;
            }
            return false;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }
    
    @Override
    public boolean stopStreaming() {
        isStreaming = false;
        println("BoardESP32: Stopped streaming");
        return true;
    }
    
    @Override
    public void update() {
        if (isStreaming && serial_ESP32 != null && serial_ESP32.available() > 0) {
            processSerialData();
        }
    }
    
    private void processSerialData() {
        while (serial_ESP32.available() > 0) {
            byte incomingByte = (byte)serial_ESP32.read();
            
            if (lookingForStart) {
                if (incomingByte == (byte)0xA0) { // Start byte
                    packetBuffer[0] = incomingByte;
                    bufferIndex = 1;
                    lookingForStart = false;
                }
            } else {
                packetBuffer[bufferIndex] = incomingByte;
                bufferIndex++;
                
                if (bufferIndex >= 33) { // Complete packet received
                    if (packetBuffer[32] == (byte)0xC0) { // Valid end byte
                        parsePacket();
                    }
                    lookingForStart = true;
                    bufferIndex = 0;
                }
            }
        }
    }
    
    private void parsePacket() {
        // Extract sample number
        int sampleNumber = packetBuffer[1] & 0xFF;
        
        // Extract 8 channels of 24-bit data
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            int baseIndex = 2 + ch * 3;
            
            // Combine 3 bytes into 24-bit signed integer
            int value = (packetBuffer[baseIndex] << 16) | 
                       ((packetBuffer[baseIndex + 1] & 0xFF) << 8) | 
                       (packetBuffer[baseIndex + 2] & 0xFF);
            
            // Sign extend from 24-bit to 32-bit
            if ((value & 0x800000) != 0) {
                value |= 0xFF000000;
            }
            
            // Convert to microvolts (similar to OpenBCI scaling)
            double microvolts = value * 0.02235; // Approximate OpenBCI scaling factor
            
            // Store in circular buffer
            dataBuffer[ch][writeIndex] = microvolts;
        }
        
        writeIndex = (writeIndex + 1) % bufferSize;
        lastSampleTime = millis();
    }
    
    @Override
    protected double[][] getNewDataInternal() {
        // Return recent data samples
        int samplesToReturn = min(10, bufferSize); // Return up to 10 samples
        double[][] result = new double[NUM_CHANNELS][samplesToReturn];
        
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            for (int i = 0; i < samplesToReturn; i++) {
                int index = (writeIndex - samplesToReturn + i + bufferSize) % bufferSize;
                result[ch][i] = dataBuffer[ch][index];
            }
        }
        
        return result;
    }
    
    @Override
    public int getSampleRate() {
        return SAMPLE_RATE;
    }
    
    @Override
    public int[] getEXGChannels() {
        int[] channels = new int[NUM_CHANNELS];
        for (int i = 0; i < NUM_CHANNELS; i++) {
            channels[i] = i;
        }
        return channels;
    }
    
    @Override
    public void setEXGChannelActive(int channelIndex, boolean active) {
        if (channelIndex >= 0 && channelIndex < NUM_CHANNELS) {
            channelActive[channelIndex] = active;
        }
    }

    @Override
    public boolean isEXGChannelActive(int channelIndex) {
        if (channelIndex >= 0 && channelIndex < NUM_CHANNELS) {
            return channelActive[channelIndex];
        }
        return false;
    }

    @Override
    protected void addChannelNamesInternal(String[] channelNames) {
        // Use default channel names
    }

    @Override
    public Pair<Boolean, String> sendCommand(String command) {
        return new ImmutablePair<Boolean, String>(false, "Commands not supported for ESP32 board");
    }
    
    @Override
    public void insertMarker(int value) {
        // ESP32 doesn't support markers
        println("BoardESP32: Markers not supported");
    }

    @Override
    public void insertMarker(double value) {
        // ESP32 doesn't support markers
        println("BoardESP32: Markers not supported");
    }

    // --- AccelerometerCapableBoard Interface Methods ---
    
    @Override
    public void setAccelerometerActive(boolean active) {
        // ESP32 board doesn't support accelerometer control
    }

    @Override
    public boolean isAccelerometerActive() {
        return false; // ESP32 doesn't have accelerometer by default
    }

    @Override
    public boolean canDeactivateAccelerometer() {
        return false;
    }

    @Override
    public int[] getAccelerometerChannels() {
        return emptyArray; // No accelerometer channels
    }

    @Override
    public int getAccelSampleRate() {
        return 0; // No accelerometer
    }

    // --- AnalogCapableBoard Interface Methods ---

    @Override
    public void setAnalogActive(boolean active) {
        // ESP32 board doesn't support analog channel control
    }

    @Override
    public boolean isAnalogActive() {
        return false; // No separate analog channels
    }

    @Override
    public boolean canDeactivateAnalog() {
        return false;
    }

    @Override
    public int[] getAnalogChannels() {
        return emptyArray; // No separate analog channels
    }

    @Override
    public int getAnalogSampleRate() {
        return 0; // No separate analog channels
    }

    // --- DigitalCapableBoard Interface Methods ---

    @Override
    public void setDigitalActive(boolean active) {
        // ESP32 board doesn't support digital channel control
    }

    @Override
    public boolean isDigitalActive() {
        return false; // No digital channels
    }

    @Override
    public boolean canDeactivateDigital() {
        return false;
    }

    @Override
    public int[] getDigitalChannels() {
        return emptyArray; // No digital channels
    }

    @Override
    public int getDigitalSampleRate() {
        return 0; // No digital channels
    }

    // --- ImpedanceSettingsBoard Interface Methods ---
    
    @Override
    public void setCheckingImpedance(int channel, boolean active) {
        // ESP32 board doesn't support impedance checking
    }

    @Override
    public boolean isCheckingImpedance(int channel) {
        return false; // No impedance checking
    }
    
    @Override
    public Integer isCheckingImpedanceOnChannel() {
        return null; // Not checking impedance
    }
}
