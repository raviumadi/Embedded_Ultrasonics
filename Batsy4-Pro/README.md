# BATSY4-PRO (Teensy 4.x) – Ultrasonic Heterodyne + Rolling Pre-Record

## Overview

**BATSY4-PRO** is a high-rate (192 kHz) ultrasonic audio capture and monitoring system built on **Teensy 4.x** with the **Teensy Audio Library** and an SSD1306 OLED UI.  
It combines live heterodyne monitoring, a rolling 5-second multi-channel ring buffer, and one-touch SD capture.

Originally designed for bat acoustics work, this code can be adapted for other ultrasonic or high-speed audio projects.

---

## Features

- **Live Heterodyne Monitoring**
  - Mixes ultrasonic channel down to audible using a carrier oscillator.
  - Adjustable carrier frequency (10–85 kHz).
  - Adjustable output gain (0–100%).

- **Rolling 5-Second, 4-Channel Ring Buffer**
  - Stored in EXTMEM (PSRAM) for high-speed operation.
  - Captures “pre-record” data before you press record.

- **One-Touch Capture to SD**
  - **Tap Button** → save last 5 seconds to WAV.
  - **Hold Button** → save 5 seconds pre-buffer + up to 10 seconds live.

- **OLED User Interface**
  - Displays carrier frequency, output gain, and edit mode.
  - Rotary encoder edits parameters; push to toggle edit mode.

---

## Controls

| Control                   | Function                                                     |
| ------------------------- | ------------------------------------------------------------ |
| **Rotary Encoder (SW)**   | Toggle between **FREQ** and **VOL** edit modes.              |
| **Rotary Encoder Rotate** | Adjust frequency (FREQ mode) or volume (VOL mode).           |
| **Tap Button**            | Save last 5 seconds from buffer.                             |
| **Hold Button**           | Save 5 seconds pre-buffer + live forward capture (up to 10 seconds). |

---

## Wiring (Teensy 4.x)

| Component        | Pin(s)   | Notes                               |
| ---------------- | -------- | ----------------------------------- |
| **OLED SSD1306** | SDA/SCL  | I²C @ 0x3C, internal pull-ups OK    |
| **Hold Button**  | 40       | Active-LOW, `INPUT_PULLUP`          |
| **Tap Button**   | 41       | Active-LOW, `INPUT_PULLUP`          |
| **Encoder A**    | 36       | `INPUT_PULLUP`                      |
| **Encoder B**    | 37       | `INPUT_PULLUP`                      |
| **Encoder SW**   | 38       | `INPUT_PULLUP`                      |
| **Encoder COM**  | GND      |                                     |
| **SD**           | Built-in | Teensy 4.1 onboard slot recommended |

---

## File Format

- **WAV** — 16-bit PCM, 4 channels interleaved, 192000 Hz.
- Temporary file: `/TEMP.WAV` (renamed on close).
- Only channel 1 is used for heterodyne playback.

---

## Performance Notes

- Use a **fast SD card** (Class 10 or better) for reliable 192 kHz writes.
- Ring buffer in EXTMEM prevents RAM starvation at high sample rates.
- If encoder direction feels reversed, invert direction in `loop()`.

---

## Limitations

- No digital filtering on heterodyne output — add LPF if needed.
- Output is mono (duplicated to L/R), derived from channel 1.
- Encoder is polled, so rapid spins during SD writes may miss steps.

---

## Example Data

See folder `field_recordings` for bat call sequences recorded using the Batsy4-Pro

## License

This work is licensed under a [Creative Commons Attribution–NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).

---

## Disclaimer

This code is provided **"as is"** without any warranties, express or implied.  
The author makes no guarantee regarding correctness, safety, or suitability for any of your particular purposes.  
You are responsible for testing and validating the code before use — especially in safety-critical or regulatory applications.

Use at your own risk.