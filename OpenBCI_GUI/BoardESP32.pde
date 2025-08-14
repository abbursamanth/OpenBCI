// ESP32 Board Implementation - Serial Communication Only
// Follows BoardCyton pattern for BrainFlow compatibility

import brainflow.*;

class BoardESP32 extends BoardBrainFlow implements AccelerometerCapableBoard, AnalogCapableBoard, DigitalCapableBoard, ImpedanceSettingsBoard {

    private String serialPort = "COM8"; // Default ESP32 port

    // Constructor
    public BoardESP32() {
        super();
    }

    public BoardESP32(String serialPort) {
        super();
        this.serialPort = serialPort;
    }

    // Implement mandatory abstract functions from BoardBrainFlow
    @Override
    protected BrainFlowInputParams getParams() {
        BrainFlowInputParams params = new BrainFlowInputParams();
        params.serial_port = serialPort;
        println("BoardESP32: Using serial port: " + serialPort);
        return params;
    }

    @Override
    public BoardIds getBoardId() {
        return BoardIds.CYTON_BOARD; // Use Cyton board ID for compatibility
    }

    // Minimal required interface implementations
    
    // --- AccelerometerCapableBoard Interface Methods ---
    @Override
    public void setAccelerometerActive(boolean active) { }
    
    @Override
    public boolean isAccelerometerActive() { return false; }
    
    @Override
    public boolean canDeactivateAccelerometer() { return false; }
    
    @Override
    public int[] getAccelerometerChannels() { return new int[0]; }
    
    @Override
    public int getAccelSampleRate() { return getSampleRate(); }
    
    @Override
    public List<double[]> getDataWithAccel(int maxSamples) {
        return getDataDefault(maxSamples);
    }

    // --- AnalogCapableBoard Interface Methods ---
    @Override
    public void setAnalogReadOn(int pin, boolean on) { }
    
    @Override
    public boolean isAnalogActive(int pin) { return false; }
    
    @Override
    public boolean canDeactivateAnalog() { return false; }
    
    @Override
    public int[] getAnalogChannels() { return new int[0]; }

    // --- DigitalCapableBoard Interface Methods ---
    @Override
    public void setDigitalReadOn(int pin, boolean on) { }
    
    @Override
    public boolean isDigitalActive(int pin) { return false; }
    
    @Override
    public boolean canDeactivateDigital() { return false; }
    
    @Override
    public int[] getDigitalChannels() { return new int[0]; }

    // --- ImpedanceSettingsBoard Interface Methods ---
    @Override
    public boolean isCheckingImpedance() { return false; }
}
