# Espertillio

## ESP32 Based <u>H</u>eterodyne, <u>R</u>ecord and <u>P</u>layback

A compact ESP32 sketch that lets you **listen to ultrasonic signals** (e.g., bat calls) by **heterodyning** them to the audible range, **generate/play** synthetic bat calls (FM or CF), and **record** raw mono audio to **WAV on microSD** at **192 kHz / 16-bit**.

---

## Features

- **Live heterodyne monitoring**  
  I²S input is mixed with a cosine at `freqShiftHz` (default 38 kHz) to shift ultrasound into hearing range.
- **Bat call synthesis & playback**  
  - **FM**: linear down-sweep (default 40 kHz → 5 kHz)  
  - **CF**: constant frequency (default 40 kHz) with optional Doppler resampling  
  Plays out via I²S at 192 kHz mono.
- **High-rate recording to WAV**  
  Mono 16-bit PCM @ 192,000 samples/s to `/REC###.WAV` on microSD.  
  Proper WAV header is finalised on stop.
- **Simple, dependable I/O**  
  Uses the ESP32 I²S driver (`driver/i2s.h`) with APLL and a ring buffer for SD writes.

---

## Hardware

- **ESP32** development board (Arduino core)
- **Ultrasonic microphone front-end** or I²S ADC for input (wired to I²S IN)
- **I²S DAC/codec** or amplifier for output (wired to I²S OUT)
- **microSD card** (SPI mode)
- 3 × **momentary switches** (Play, Record, Mode Select) — pull-ups enabled in code
- Stable 5V/3.3V supply (high sample rates + SD writes are power-sensitive)

### Pinout (defaults in the sketch)

**I²S Input (I2S_NUM_0)**  
- **BCK**: GPIO 32  
- **WS** : GPIO 33  
- **DI** : GPIO 35

**I²S Output (I2S_NUM_1)**  
- **BCK**: GPIO 25  
- **WS** : GPIO 27  
- **DO** : GPIO 26

**microSD (SPI mode)**  
- **CS** : GPIO 5  
- **SCK/MOSI/MISO**: Board VSPI defaults (often 18/23/19)

**Buttons**  
- **BUTTON_PIN** (GPIO 4): Play generated call (FM/CF chosen by SWITCH_PIN)  
- **RECORD_BUTTON** (GPIO 2): LOW = start, HIGH = stop & save WAV  
- **SWITCH_PIN** (GPIO 16): LOW = CF, HIGH = FM (for playback button)

> If your hardware uses different pins, adjust the `#define` block at the top.

---

## Build & Dependencies

- **Arduino IDE** (or PlatformIO) with **ESP32** board support
- Libraries: **SD**, **SPI** (bundled with ESP32 Arduino core)
- Uses **ESP-IDF I²S** driver via `#include "driver/i2s.h"` (provided by ESP32 core)

**Board settings:**  
- CPU Frequency: default is fine  
- Flash: your board’s default  
- PSRAM: optional (not required)

---

## Configuration

Key constants (at the top of the file):

```cpp
#define SAMPLE_RATE 192000           // 192 kHz mono
const float freqShiftHz = 38000.0f;  // heterodyne LO
#define CALL_F0_HZ 40000.0f          // FM/CF base frequency
#define CALL_F1_HZ 5000.0f           // FM end frequency
#define CALL_DURATION_MS 50.0f
#define CALL_TAIL_PCT 50              // zero-padding on both sides (% of call)
#define DOPPLER_VELOCITY 0.0f         // m/s (CF only)
const float CALL_VOLUME_SCALE = 0.1f // playback amplitude
#define RING_BUFFER_SIZE 8192         // SD write chunk (bytes)
```

Tweak freqShiftHz to target different ultrasonic bands.



## **Controls**

- **Play call** (BUTTON_PIN, GPIO 4):

  

  - **SWITCH_PIN LOW** → Play **CF** call (with optional Doppler)
  - **SWITCH_PIN HIGH** → Play **FM** sweep

  

- **Record** (RECORD_BUTTON, GPIO 2):

  

  - **LOW** → Start recording to /REC###.WAV
  - **HIGH** → Stop + finalise WAV header

  

> While **recording**, live heterodyne output is paused to prioritise I/O.

------



## **How it Works**

- **I²S**

  - I2S_NUM_0 configured as **RX** (mono, left slot)
  - I2S_NUM_1 configured as **TX** (mono, left slot)
  - APLL and fixed MCLK (256 × Fs) for a clean 192 kHz clock

- **Heterodyne**

  Read input frames → multiply by cos(2π·freqShiftHz·t) → clip/scale → write to I²S OUT

- **Recording**

  Raw input samples → RAM ring buffer → flush to SD in chunks

  On stop, the WAV header is written with the accurate data size

- **Synthesis**

  - **FM**: linear chirp with Hann window; centred using zero padding
  - **CF**: single tone with Hann window; optional Doppler resampling

------



## **File Naming & Format**

- Output files: /REC000.WAV, /REC001.WAV, …
- Format: **WAV PCM**, **16-bit**, **mono**, **192,000 Hz**

------



## **Performance Tips**

- Use a **fast microSD card** (A1/A2 or better)
- If you hear dropouts, **increase RING_BUFFER_SIZE**
- Keep I²S wiring short, shielded, and well-grounded
- Reduce CALL_VOLUME_SCALE or input gain if clipping occurs
- Ensure a **stable power supply**

------



## **Troubleshooting**

- **No audio out**: check I²S OUT wiring and DAC path
- **No input**: confirm mic/ADC I²S IN wiring and format
- **WAV won’t play**: stop recording properly to finalise header
- **SD errors**: reformat card, try faster card, increase RING_BUFFER_SIZE



