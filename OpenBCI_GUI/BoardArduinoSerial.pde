// Arduino Serial Board Implementation
// Reads CSV data from Arduino and converts it to OpenBCI GUI format

import brainflow.*;
import processing.serial.*;

class BoardArduinoSerial extends BoardBrainFlow implements AccelerometerCapableBoard {

    private int numChannels = 6; // Arduino has 6 analog channels
    private volatile boolean[] activeChannels = null;
    private Serial arduinoSerial;
    private String serialPort = "COM9";
    private boolean isConnected = false;
    private ArrayList<String[]> dataBuffer;
    private int[] accelChannelsCache = null;

    public BoardArduinoSerial() {
        super();
        activeChannels = new boolean[numChannels];
        for (int i = 0; i < numChannels; i++) {
            activeChannels[i] = true;
        }
        dataBuffer = new ArrayList<String[]>();
    }

    public BoardArduinoSerial(String port) {
        this();
        this.serialPort = port;
    }

    @Override
    protected BrainFlowInputParams getParams() {
        BrainFlowInputParams params = new BrainFlowInputParams();
        // We'll handle serial communication manually
        return params;
    }

    @Override
    public BoardIds getBoardId() {
        return BoardIds.SYNTHETIC_BOARD; // Use synthetic board ID for compatibility
    }

    @Override
    public boolean initializeInternal() {
        try {
            println("BoardArduinoSerial: Connecting to Arduino on " + serialPort);
            arduinoSerial = new Serial(ourApplet, serialPort, 9600);
            arduinoSerial.bufferUntil('\n'); // Buffer until newline
            isConnected = true;
            println("BoardArduinoSerial: Connected successfully!");
            return true;
        } catch (Exception e) {
            println("BoardArduinoSerial: Failed to connect - " + e.getMessage());
            isConnected = false;
            return false;
        }
    }

    @Override
    public void uninitializeInternal() {
        if (arduinoSerial != null) {
            arduinoSerial.stop();
            arduinoSerial = null;
        }
        isConnected = false;
        println("BoardArduinoSerial: Disconnected");
    }

    @Override
    public int[] getEXGChannels() {
        int[] channels = new int[numChannels];
        for (int i = 0; i < numChannels; i++) {
            channels[i] = i; // Channels 0-5
        }
        return channels;
    }

    @Override
    public void setEXGChannelActive(int channelIndex, boolean active) {
        if (channelIndex >= 0 && channelIndex < numChannels) {
            activeChannels[channelIndex] = active;
        }
    }

    @Override
    public boolean isEXGChannelActive(int channelIndex) {
        if (channelIndex >= 0 && channelIndex < numChannels) {
            return activeChannels[channelIndex];
        }
        return false;
    }

    @Override
    public int getSampleRate() {
        return 50; // 50Hz from Arduino
    }

    @Override
    public int getNumEXGChannels() {
        return numChannels;
    }

    // Process incoming serial data
    public void processSerialData() {
        if (arduinoSerial != null && arduinoSerial.available() > 0) {
            try {
                String line = arduinoSerial.readStringUntil('\n');
                if (line != null) {
                    line = line.trim();
                    // Skip header lines and empty lines
                    if (!line.startsWith("Arduino") && !line.startsWith("Format") && 
                        !line.startsWith("Starting") && line.length() > 0) {
                        
                        String[] values = line.split(",");
                        if (values.length >= 7) { // Sample number + 6 channels
                            // Store the channel data (skip sample number at index 0)
                            String[] channelData = new String[6];
                            for (int i = 0; i < 6; i++) {
                                channelData[i] = values[i + 1]; // Skip sample number
                            }
                            synchronized (dataBuffer) {
                                dataBuffer.add(channelData);
                                // Keep buffer size reasonable
                                if (dataBuffer.size() > 1000) {
                                    dataBuffer.remove(0);
                                }
                            }
                        }
                    }
                }
            } catch (Exception e) {
                println("BoardArduinoSerial: Error reading serial data - " + e.getMessage());
            }
        }
    }

    @Override
    public List<double[]> getDataDefault(int maxSamples) {
        List<double[]> result = new ArrayList<double[]>();
        
        // Process any new serial data
        processSerialData();
        
        synchronized (dataBuffer) {
            int samplesToReturn = Math.min(maxSamples, dataBuffer.size());
            
            for (int i = 0; i < samplesToReturn; i++) {
                String[] sample = dataBuffer.get(i);
                double[] row = new double[numChannels];
                
                for (int ch = 0; ch < numChannels && ch < sample.length; ch++) {
                    try {
                        row[ch] = Double.parseDouble(sample[ch]);
                    } catch (NumberFormatException e) {
                        row[ch] = 0.0; // Default value if parsing fails
                    }
                }
                result.add(row);
            }
            
            // Remove processed samples
            for (int i = 0; i < samplesToReturn; i++) {
                dataBuffer.remove(0);
            }
        }
        
        return result;
    }

    // Accelerometer interface (not supported, return empty)
    @Override
    public void setAccelerometerActive(boolean active) { }
    
    @Override
    public boolean isAccelerometerActive() { return false; }
    
    @Override
    public boolean canDeactivateAccelerometer() { return false; }
    
    @Override
    public int[] getAccelerometerChannels() { 
        if (accelChannelsCache == null) {
            accelChannelsCache = new int[0];
        }
        return accelChannelsCache; 
    }
    
    @Override
    public int getAccelSampleRate() { return getSampleRate(); }
    
    @Override
    public List<double[]> getDataWithAccel(int maxSamples) {
        return getDataDefault(maxSamples);
    }
}
