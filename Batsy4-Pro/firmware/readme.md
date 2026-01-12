# Batsy4-Pro Firmware Upload

This folder contains a **precompiled firmware binary** for the Batsy4-Pro ultrasonic recorder, built for the **Teensy 4.1** microcontroller.  
Uploading this firmware does **not** require the Arduino IDE or source code.

## Requirements
- Teensy 4.1 board
- USB cable
- Teensy Loader application

Download Teensy Loader from:  
https://www.pjrc.com/teensy/loader.html

## Uploading the Firmware

1. Connect the Teensy 4.1 to your computer using a USB cable.
2. Open the **Teensy Loader** application.
3. Click **File → Open HEX File** and select: `batsy4pro.ino.hex`
4. Press the **physical program button** on the Teensy 4.1 once.
5. The firmware will upload automatically.

When the upload is complete, the Teensy will reboot and the firmware will start running.

## Notes
- These binaries were compiled specifically for **Teensy 4.1**.
- No compilation or configuration is required for standard operation.
- Source code, documentation, and advanced build instructions are available in the main repository.