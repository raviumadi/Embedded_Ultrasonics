# Remote Trigger for Batsy4-Pro

## ESP-NOW-Based Wireless Recording Trigger

This subdirectory provides a compact wireless trigger solution for triggering the BATSY4-Pro recorder using ESP32 microcontrollers and the **ESP-NOW** communication protocol.

The system consists of two components:

- sender.ino: A hand-held pushbutton transmitter
- receiver.ino: A base station module that receives the trigger and sends **output pulses** to **digital pins**, which are fed into the BATSY4-Pro recorder (essentially supplementary to the push buttons for two recording modes)



## **Features**

- **Reliable Wireless Triggering** using ESP-NOW protocol
- **Dual Channel Output**: trigger pulses on two pins, TAP and HOLD recording modes, which save the ring buffer and buffer+N seconds forward recording
- **Remote Range**: Works over ~10m

## **Hardware Requirements**

### **Sender**

- 1× ESP32 Dev Board
- 2× Pushbutton (connected to two different GPIO)

### **Receiver**

- 1× ESP32 Dev Board
- 2× Digital output pins (e.g., GPIO 25 and GPIO 26) connected to trigger input ports on BATSY4-Pro

## **Installation**

1. **Install Arduino IDE and ESP32 Board Support**

   - [Install ESP32 core via board manager](https://github.com/espressif/arduino-esp32)

   

2. **Open and Flash sender.ino to the transmitter ESP32**

   - Adjust peerAddress[] to match the MAC address of the receiver
   - Use Serial.print(WiFi.macAddress()); on the receiver to find it

   

3. **Open and Flash receiver.ino to the receiver ESP32**

   - Set your selected pins
   - Get your MAC address by running this with the serial monitor running and then flash the sender.ino

   

4. **Power both devices**

   - Pressing the button on the transmitter sends a trigger signal
   - Receiver outputs 100 ms HIGH pulses on the corresponding trigger pins

## **Notes**

- Ensure both ESP32 devices are within communication range
- Test and confirm the MAC addresses are correctly paired
- You may extend the system for more channels or longer pulses if required

## **License**

This work is licensed under a  
[Creative Commons Attribution–NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).

