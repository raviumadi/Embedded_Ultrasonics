# Esperdyne

## ESP32 Heterodyne Monitor + WAV Playback

This project turns an ESP32 into a **real-time ultrasonic heterodyne monitor** and **WAV file player**.

## Features
- **Live Heterodyne Monitoring**: Down-converts ultrasonic input (I²S RX) to audible range at 38 kHz shift, or chosen frequency.
- **WAV Playback**: Plays a PCM 16-bit mono 192 kHz `.wav` file from SD card when a button is pressed.
- **Automatic Return to Monitoring**: After playback, reverts to live heterodyne mode.

## Hardware Requirements
- ESP32 development board.
- I²S microphone/ADC for ultrasonic input.
- I²S DAC + amplifier + speaker for output.
- microSD card module.
- Push button (active-LOW).

## Pin Mapping
**I²S Input (I2S_NUM_0)**  
- BCK: GPIO 32  
- WS: GPIO 33  
- DATA: GPIO 35  

**I²S Output (I2S_NUM_1)**  
- BCK: GPIO 25  
- WS: GPIO 27  
- DATA: GPIO 26  

**Control**  
- BUTTON_PIN: GPIO 4 (active-LOW)

## How It Works
1. **On Boot**:  
   - Initialises I²S RX/TX at 192 kHz, 16-bit mono.  
   - Looks for the first `.wav` in the SD card root (ignores hidden files).  

2. **Live Mode**:  
   - Continuously reads from I²S IN.  
   - Multiplies samples by a 38 kHz cosine (heterodyne - choose your frequency).  
   - Sends output to I²S OUT.

3. **Playback Mode**:  
   - Triggered by pressing the button.  
   - Streams WAV file data to I²S OUT until EOF.  
   - Returns to live heterodyne monitoring.

## Usage
- Wire the hardware as per pin mapping.  
- Place a PCM 16-bit mono 192 kHz `.wav` file on the SD card.  (Make sure your file is indeed single channel)
- Upload and run the sketch.  
- Listen to live heterodyned audio; press the button to play the stored WAV.

## License
This project is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)** license.

## Disclaimer
This code is provided *“as is”* without warranty. You are responsible for verifying functionality and ensuring safe and legal operation, especially in field or wildlife applications.
