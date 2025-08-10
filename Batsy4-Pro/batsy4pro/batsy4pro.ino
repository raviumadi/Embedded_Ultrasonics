/*
================================================================================
Project: BATSY4-PRO (Teensy 4.x) – Ultrasonic Heterodyne + Rolling Pre-Record
Author : Ravi Umadi
Target : Teensy 4.0 / 4.1 + Teensy Audio Library + SSD1306 OLED
Date   : 2025-08-07

OVERVIEW
--------
This sketch implements a high-rate (192 kHz) audio pipeline for ultrasonic work
(e.g., bat acoustics) with:

  1) Live heterodyne monitoring
     - Left input channel is multiplied by a local oscillator (carrier) to
       shift ultrasonic content down to audible.
     - Output gain is user-adjustable.

  2) Rolling 5 s, 4-channel ring buffer in EXTMEM
     - Continuously stores I²S input frames for “pre-record” capture.

  3) One-touch capture to SD (WAV)
     - TAP button: save the LAST 5 seconds from the ring buffer.
     - HOLD button: save 5 s “pre” + up to 10 s “post”. Customisable

  4) On-device UI
     - 128×64 SSD1306 OLED shows carrier frequency, output volume, and mode.
     - Rotary encoder edits parameters; push to switch between FREQ and VOL.

WHAT THE KNOBS & BUTTONS DO
---------------------------
- Rotary Encoder (pins 36=A, 37=B, 38=SW)
  • Press (SW): toggle edit mode → FREQ or VOL.
  • Rotate:
      - In FREQ mode: carrier changes in ±5 kHz steps (10–85 kHz limits).
      - In VOL  mode: output gain changes in ±5% steps (0–100%).
  • If rotation direction feels reversed, see comment in `loop()` to flip.

- TAP button (pin 41)
  • Immediately writes the *previous* 5 s (all 4 interleaved channels)
    from the ring buffer to a new WAV on SD.

- HOLD button (pin 40)
  • Writes 5 s pre-buffer, then continues writing live data up to 10 s

SIGNAL FLOW (high level)
------------------------
I2S In (2x stereo = 4 channels)
   → AudioRecordQueue (L1,R1,L2,R2)
   → Copy to EXTMEM ring buffer (interleaved int16)
   → Heterodyne: out[n] = ch1[n] * sin(phase) * outGain
   → AudioPlayQueue → I2S Out L/R (mirrored mono)

HETERODYNE DETAILS
------------------
- `carrierFreq` (Hz) sets oscillator frequency (default 45 kHz).
- `phaseIncrement = 2π * carrierFreq / sampleRate`.
- Only ch1 (queueL1) is mixed to produce the audible output stream.
- Output volume is scaled by `outGain` (0.0–1.0) and hard-limited to int16.
- Use a powered headphone to raise volume further.

AUDIO / CLOCKING
----------------
- Sample rate forced to 192000 Hz (`AUDIO_SAMPLE_RATE_EXACT`).
- `setI2SFreqBoth()` reprograms Teensy SAI1/SAI2 clocks so both I2S buses
  run at 192 kHz in lockstep (Teensy-specific clock registers).
- `AudioMemory(120)` allocates Teensy Audio Library blocks.

STORAGE FORMAT
--------------
- Temporary file: `/TEMP.WAV` (header space reserved).
- On close, header is written and file is renamed to `/BAT###.WAV`.
- PCM WAV format: 16-bit, 4 channels interleaved, 192000 Hz.

OLED UI (SSD1306 @ 0x3C)
------------------------
- Always shows:
    CARR: (kHz)   VOL: (%)   MODE: FREQ|VOL
- Updates instantly after edits or capture actions.

ENCODER IMPLEMENTATION (reliable on pins 36/37/38)
--------------------------------------------------
- Polled quadrature (no interrupts) using `Bounce` on A/B.
- Transition table decodes direction; `TICKS_PER_DETENT` default is 4.
  If your encoder produces 2 transitions per detent, set to 2.

WIRING SUMMARY (Teensy 4.x)
---------------------------
- SSD1306 OLED: I²C @ 0x3C (SDA/SCL per Teensy; internal pull-ups OK).
- Buttons (active-LOW with INPUT_PULLUP):
    HOLD → pin 40
    TAP  → pin 41
- Rotary Encoder:
    A   → pin 36 (INPUT_PULLUP)
    B   → pin 37 (INPUT_PULLUP)
    SW  → pin 38 (INPUT_PULLUP)
    COM → GND
- SD: Onboard `BUILTIN_SDCARD` (Teensy 4.1) or wired SD if different board.

KEY CONSTANTS TO TUNE
---------------------
- `FREQ_STEP` (default 5000.0f)    : encoder step for frequency (Hz).
- Min/Max carrier (`FREQ_MIN/MAX`) : 10–85 kHz by default (avoid aliasing).
- `VOL_STEP` (default 0.05f)       : encoder step for volume (5%).
- `ringBufferSeconds` (default 5)  : rolling buffer length (seconds). Can be increased upto 10s if you use 16MB PSRAM. Check Teensy 4.1 documentation.
- `numChannels` (fixed 4)          : interleaved channel count in ring/WAV. You may use more I2S ADC by multiplexing the ports. Upto 8 channel is possible. But must be optimised for realtime processing. 

PERFORMANCE NOTES
-----------------
- Ring buffer lives in EXTMEM (PSRAM) — fast enough for 192 kHz × 4 ch.
- SD writes are chunked during capture; long forward records rely on the
  loop keeping up with I/O. Fast SD media recommended.
- The output path is mono (duplicated to L/R), derived only from ch1. You can select the channel in processing loop function.
- `totalSamplesWritten` tracks readiness for the 5 s pre-buffer logic.

LIMITATIONS / GOTCHAS
---------------------
- No antialias or band-limit filtering on the heterodyne product; The audio output is quite clean and no hissing noise is present. But if you amplifiy the analogue signal, additional bandpass may be necessary to keep the audio clean.
- The encoder is polled; if UI misses steps under extreme SD loads, reduce
  `TICKS_PER_DETENT`, increase UI update rate, or add a dedicated task.
- The four recorded channels are raw as-captured; only ch1 is processed to
  audio output in this example.

TROUBLESHOOTING
---------------
- OLED shows “SD Failed!”: check card, formatting, or Teensy 4.1 SD slot.
- No encoder effect: verify pins 36/37/38 and GND; try `TICKS_PER_DETENT=2`;
  flip direction by uncommenting the line inside `loop()` (see code).
- No audio out: confirm AudioMemory, clock setup, and I2S wiring.
- WAV won’t open: ensure file closed (after capture), header gets written.

DISCLAIMER
----------
This code is provided **“as is”** without warranty of any kind, express or
implied, including but not limited to the warranties of merchantability,
fitness for a particular purpose, and non-infringement. You are solely
responsible for verifying correctness, safety, and suitability for your
application—especially in any safety-critical, field, wildlife, or
regulatory contexts. The author(s) shall not be liable for any claim,
damages, or other liability, whether in an action of contract, tort, or
otherwise, arising from, out of, or in connection with the software or the
use or other dealings in the software. **Use at your own risk.**

License

This work is licensed under a  
[Creative Commons Attribution–NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).
================================================================================
*/

#include <Arduino.h>
#include <Audio.h>
#include <SD.h>
#include <Wire.h>
#include <stdint.h>
#include <math.h>
#include <cstdint>
#include <utility/imxrt_hw.h>
#include "core_pins.h"
#include <Bounce.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#undef AUDIO_SAMPLE_RATE_EXACT
#define AUDIO_SAMPLE_RATE_EXACT 192000.0f

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Buttons (unchanged) ---
const int buttonHoldPin = 40;
const int buttonTapPin  = 41;
Bounce buttonHold = Bounce(buttonHoldPin, 10);
Bounce buttonTap  = Bounce(buttonTapPin, 10);

// --- Rotary Encoder (polled) ---
const int ROT_A  = 36;   // Pin A
const int ROT_B  = 37;   // Pin B
const int ROT_SW = 38;   // Push switch
Bounce encA(ROT_A, 2);   // light debounce
Bounce encB(ROT_B, 2);
Bounce rotSW(ROT_SW, 10);

// Quadrature decoder (polled)
int32_t encoderTicks = 0;         // accumulated transitions
uint8_t encLastState = 0;         // last 2-bit AB state
const int TICKS_PER_DETENT = 4;   // adjust to 2 if your encoder is 2/transitions-detent

// Editable parameters via encoder
enum EditMode { EDIT_FREQ = 0, EDIT_VOL = 1 };
EditMode editMode = EDIT_FREQ;

const float FREQ_MIN   = 10000.0f;
const float FREQ_MAX   = 85000.0f;
const float FREQ_STEP  = 5000.0f;     // per detent

volatile float carrierFreq = 45000.0f;
volatile float outGain     = 0.50f;            // 0.0–1.0
const float VOL_STEP   = 0.05f;       // per detent
const float VOL_MIN    = 0.0f;
const float VOL_MAX    = 1.0f;

const float twoPi = 6.28318530718f;
float phase = 0;
float phaseIncrement = 0;

// --- Audio + Ring Buffer ---
const int sampleRate = 192000;
const int bytesPerSample = 2;
const int numChannels = 4;
const int ringBufferSeconds = 5;
const uint32_t ringBufferSamples = sampleRate * ringBufferSeconds * numChannels;
volatile uint64_t totalSamplesWritten = 0;

EXTMEM int16_t ringBuffer[ringBufferSamples];
volatile uint32_t ringWriteIndex = 0;

File file;
uint32_t sampleCount = 0;
int fileIndex = 0;
const char *TEMP_FILENAME = "/TEMP.WAV";

// --- Teensy Audio objects ---
AudioInputI2S   i2sInput1;
AudioRecordQueue queueL1, queueR1;
AudioInputI2S2  i2sInput2;
AudioRecordQueue queueL2, queueR2;
AudioPlayQueue   outputQueue;
AudioOutputI2S   i2sOut;

AudioConnection patchCord1(i2sInput1, 0, queueL1, 0);
AudioConnection patchCord2(i2sInput1, 1, queueR1, 0);
AudioConnection patchCord3(i2sInput2, 0, queueL2, 0);
AudioConnection patchCord4(i2sInput2, 1, queueR2, 0);
AudioConnection patchOutL(outputQueue, 0, i2sOut, 0);
AudioConnection patchOutR(outputQueue, 0, i2sOut, 1);

extern "C" void set_audioClock(uint32_t nfact, uint32_t nmult, uint32_t ndiv, bool force = false);

void setI2SFreqBoth(int freq) {
  int n1 = 4;
  int n2 = 1 + (24000000 * 27) / (freq * 256 * n1);
  double C = ((double)freq * 256 * n1 * n2) / 24000000;
  int c0 = (int)C, c2 = 10000, c1 = (int)(C * c2) - (c0 * c2);
  set_audioClock(c0, c1, c2, true);
  CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK)) | CCM_CS1CDR_SAI1_CLK_PRED(n1 - 1) | CCM_CS1CDR_SAI1_CLK_PODF(n2 - 1);
  CCM_CS2CDR = (CCM_CS2CDR & ~(CCM_CS2CDR_SAI2_CLK_PRED_MASK | CCM_CS2CDR_SAI2_CLK_PODF_MASK)) | CCM_CS2CDR_SAI2_CLK_PRED(n1 - 1) | CCM_CS2CDR_SAI2_CLK_PODF(n2 - 1);
}

// ---------- WAV helpers ----------
void writeWavHeader(File &file, uint32_t sampleRate, uint32_t numSamples) {
  file.seek(0);
  uint32_t dataSize = numSamples * numChannels * bytesPerSample;
  uint32_t chunkSize = dataSize + 36;
  uint16_t blockAlign = numChannels * bytesPerSample;
  uint32_t byteRate = sampleRate * blockAlign;
  uint8_t header[44] = {
    'R','I','F','F',
    (uint8_t)(chunkSize),(uint8_t)(chunkSize>>8),(uint8_t)(chunkSize>>16),(uint8_t)(chunkSize>>24),
    'W','A','V','E','f','m','t',' ',
    16,0,0,0,
    1,0,
    (uint8_t)(numChannels),0,
    (uint8_t)(sampleRate),(uint8_t)(sampleRate>>8),(uint8_t)(sampleRate>>16),(uint8_t)(sampleRate>>24),
    (uint8_t)(byteRate),(uint8_t)(byteRate>>8),(uint8_t)(byteRate>>16),(uint8_t)(byteRate>>24),
    (uint8_t)(blockAlign),0,
    16,0,
    'd','a','t','a',
    (uint8_t)(dataSize),(uint8_t)(dataSize>>8),(uint8_t)(dataSize>>16),(uint8_t)(dataSize>>24)
  };
  file.write(header, 44);
}

String getNextFilename() {
  char filename[20];
  while (true) {
    snprintf(filename, sizeof(filename), "/BAT%03d.WAV", fileIndex++);
    if (!SD.exists(filename)) return String(filename);
  }
}

void startNewRecording() {
  sampleCount = 0;
  file = SD.open(TEMP_FILENAME, FILE_WRITE);
  if (!file) return;
  for (int i = 0; i < 44; i++) file.write((uint8_t)0);
  file.flush();
}

void closeWavFile() {
  file.seek(0);
  writeWavHeader(file, sampleRate, sampleCount);
  file.close();
  String newFilename = getNextFilename();
  SD.rename(TEMP_FILENAME, newFilename.c_str());
  Serial.print("\xF0\x9F\x93\x81 Saved recording: ");
  Serial.println(newFilename);
}

void saveLastNSecondsFromRingBuffer(File &file, int seconds) {
  uint32_t numSamples = seconds * sampleRate * numChannels;
  uint32_t idx = (ringWriteIndex + ringBufferSamples - numSamples) % ringBufferSamples;
  for (uint32_t i = 0; i < numSamples; i++) {
    file.write((uint8_t *)&ringBuffer[idx], 2);
    sampleCount++;
    idx = (idx + 1) % ringBufferSamples;
  }
}

void dumpRingBufferToSD() {
  startNewRecording();
  saveLastNSecondsFromRingBuffer(file, 5);
  closeWavFile();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Saved 5s ring!");
  display.display();
  delay(1500);
  // Refresh UI
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("BATSY-4-PRO Ready!");
  display.setCursor(0, 16);
  display.print("CARR: ");
  display.print(carrierFreq / 1000.0, 1);
  display.println(" kHz");
  display.setCursor(0, 26);
  display.print("VOL : ");
  display.print((int)(outGain * 100));
  display.println(" %");
  display.setCursor(0, 42);
  display.print("MODE: ");
  display.println(editMode == EDIT_FREQ ? "FREQ" : "VOL");
  display.display();

  Serial.print("\xE2\x9C\x85 Dumped ring buffer: ");
  Serial.println(sampleCount);
}

// ---------- Audio processing ----------
void readAndProcessAudio() {
  if (queueL1.available() && queueR1.available() && queueL2.available() && queueR2.available()) {
    int16_t *ch1 = (int16_t *)queueL1.readBuffer();
    int16_t *ch2 = (int16_t *)queueR1.readBuffer();
    int16_t *ch3 = (int16_t *)queueL2.readBuffer();
    int16_t *ch4 = (int16_t *)queueR2.readBuffer();
    static int16_t tempOut[128];

    for (int i = 0; i < 128; i++) {
      ringBuffer[ringWriteIndex++] = ch1[i];
      ringBuffer[ringWriteIndex++] = ch2[i];
      ringBuffer[ringWriteIndex++] = ch3[i];
      ringBuffer[ringWriteIndex++] = ch4[i];
      if (ringWriteIndex >= ringBufferSamples) ringWriteIndex = 0;

      // Heterodyne (mix ch1 with carrier) with volume control
      float carrier = sinf(phase);
      phase += phaseIncrement;
      if (phase >= twoPi) phase -= twoPi;

      float mixed = (float)ch1[i] * carrier * outGain;
      if (mixed > 32767.f) mixed = 32767.f;
      if (mixed < -32768.f) mixed = -32768.f;
      tempOut[i] = (int16_t)mixed;
    }

    // count 128 frames (per channel block written above)
    totalSamplesWritten += 128UL * numChannels;

    queueL1.clear();
    queueR1.clear();
    queueL2.clear();
    queueR2.clear();

    if (outputQueue.available()) {
      int16_t *out = outputQueue.getBuffer();
      memcpy(out, tempOut, sizeof(tempOut));
      outputQueue.playBuffer();
    }
  }
}

// ---------- UI helper ----------
void drawUI() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("BATSY-4-PRO Ready!");
  display.setCursor(0, 16);
  display.print("CARR: ");
  display.print(carrierFreq / 1000.0, 1);
  display.println(" kHz");
  display.setCursor(0, 26);
  display.print("VOL : ");
  display.print((int)(outGain * 100));
  display.println(" %");
  display.setCursor(0, 42);
  display.print("MODE: ");
  display.println(editMode == EDIT_FREQ ? "FREQ" : "VOL");
  display.display();
}

// ---------- Apply encoder steps ----------
void applyEncoderSteps(int detents) {
  if (detents == 0) return;

  if (editMode == EDIT_FREQ) {
    carrierFreq += detents * FREQ_STEP;
    if (carrierFreq < FREQ_MIN) carrierFreq = FREQ_MIN;
    if (carrierFreq > FREQ_MAX) carrierFreq = FREQ_MAX;
    phaseIncrement = twoPi * carrierFreq / sampleRate;

    Serial.print("Carrier Frequency: ");
    Serial.print(carrierFreq);
    Serial.print(" Hz, phaseInc: ");
    Serial.println(phaseIncrement, 8);
  } else {
    outGain += detents * VOL_STEP;
    if (outGain < VOL_MIN) outGain = VOL_MIN;
    if (outGain > VOL_MAX) outGain = VOL_MAX;

    Serial.print("Output Volume: ");
    Serial.print(outGain * 100.0f, 0);
    Serial.println("%");
  }
  drawUI();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // --- Buttons
  pinMode(buttonHoldPin, INPUT_PULLUP);
  pinMode(buttonTapPin,  INPUT_PULLUP);

  // --- Rotary encoder pins
  pinMode(ROT_A,  INPUT_PULLUP);
  pinMode(ROT_B,  INPUT_PULLUP);
  pinMode(ROT_SW, INPUT_PULLUP);

  // Initialize last state
  uint8_t a0 = digitalRead(ROT_A) ? 1 : 0;
  uint8_t b0 = digitalRead(ROT_B) ? 1 : 0;
  encLastState = (a0 << 1) | b0;

  // Audio params
  phaseIncrement = twoPi * carrierFreq / sampleRate;

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("\xE2\x9D\x8C SSD1306 allocation failed");
    while (true) {;}
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  drawUI();

  // Audio setup
  AudioMemory(120);
  setI2SFreqBoth(sampleRate);
  queueL1.begin(); queueR1.begin();
  queueL2.begin(); queueR2.begin();
  delay(100);

  // SD
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("\xE2\x9D\x8C SD card init failed");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("SD Failed!");
    display.display();
    return;
  }
  Serial.println("\xE2\x8F\xBA Ring buffer active");
}

void loop() {
  // Update debouncers
  buttonHold.update();
  buttonTap.update();
  rotSW.update();
  encA.update();
  encB.update();

  // Trigger quadrature decode on any A/B edge
  bool anyEdge = encA.risingEdge() || encA.fallingEdge() || encB.risingEdge() || encB.fallingEdge();
  if (anyEdge) {
    uint8_t a = encA.read() ? 1 : 0;
    uint8_t b = encB.read() ? 1 : 0;
    uint8_t state = (a << 1) | b;

    // Gray-code transition table: idx = (last<<2)|current
    static const int8_t trans[16] = {
      0,  -1,  +1,   0,
      +1,  0,   0,  -1,
      -1,  0,   0,  +1,
       0, +1,  -1,   0
    };
    uint8_t idx = ((encLastState & 0x03) << 2) | (state & 0x03);
    encoderTicks += trans[idx];
    encLastState = state;

    // Convert transitions to detents
    int detents = 0;
    while (encoderTicks >= TICKS_PER_DETENT) { encoderTicks -= TICKS_PER_DETENT; detents++; }
    while (encoderTicks <= -TICKS_PER_DETENT){ encoderTicks += TICKS_PER_DETENT; detents--; }
    if (detents != 0) {
      // If direction feels reversed, uncomment next line
      // detents = -detents;
      applyEncoderSteps(detents);
    }
  }

  // Encoder push: toggle mode (FREQ <-> VOL)
  if (rotSW.fallingEdge()) {
    editMode = (editMode == EDIT_FREQ) ? EDIT_VOL : EDIT_FREQ;
    drawUI();
  }

  readAndProcessAudio();

  // TAP → Save last 5 seconds
  if (buttonTap.fallingEdge()) {
    dumpRingBufferToSD();
  }

  // HOLD → Save 5s buffer + 10s forward
  if (buttonHold.fallingEdge()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Recording...");
    display.display();

    const uint32_t minRequiredSamples = sampleRate * numChannels * 5;

    // Wait until enough samples in ring
    while (true) {
      uint64_t written;
      noInterrupts();
      written = totalSamplesWritten;
      interrupts();
      if (written >= minRequiredSamples) break;
      readAndProcessAudio();
      delay(1);
    }

    // Snapshot write index
    uint32_t fixedWritePtr;
    noInterrupts();
    fixedWritePtr = ringWriteIndex;
    interrupts();

    // Start position for 5s backfill
    uint32_t readPtr = (fixedWritePtr + ringBufferSamples - minRequiredSamples) % ringBufferSamples;

    // Start recording
    startNewRecording();
    if (!file) return;

    // Write 5s history
    uint32_t samplesToWrite = minRequiredSamples;
    const uint32_t chunkWrite = 128;
    while (samplesToWrite > 0) {
      readAndProcessAudio();
      uint32_t writeNow = min(samplesToWrite, chunkWrite);
      for (uint32_t i = 0; i < writeNow; i++) {
        file.write((uint8_t *)&ringBuffer[readPtr], 2);
        sampleCount++;
        readPtr = (readPtr + 1) % ringBufferSamples;
      }
      samplesToWrite -= writeNow;
      yield();
    }

    // Forward recording up to 10s
    const uint32_t chunkSize = 128 * numChannels * 4;
    unsigned long startTime = millis();
    const unsigned long maxDuration = 10000;

    while (millis() - startTime < maxDuration) {
      buttonTap.update();
      readAndProcessAudio();

      uint32_t writePtr;
      noInterrupts();
      writePtr = ringWriteIndex;
      interrupts();

      uint32_t available = (writePtr + ringBufferSamples - readPtr) % ringBufferSamples;
      if (available >= chunkSize) {
        for (uint32_t i = 0; i < chunkSize; i++) {
          file.write((uint8_t *)&ringBuffer[readPtr], 2);
          sampleCount++;
          readPtr = (readPtr + 1) % ringBufferSamples;
        }
      }

      if (buttonTap.fallingEdge()) break;
      yield();
    }

    // Flush tail
    uint32_t finalWritePtr;
    noInterrupts();
    finalWritePtr = ringWriteIndex;
    interrupts();
    while (readPtr != finalWritePtr) {
      file.write((uint8_t *)&ringBuffer[readPtr], 2);
      sampleCount++;
      readPtr = (readPtr + 1) % ringBufferSamples;
    }

    closeWavFile();
    queueL1.clear(); queueR1.clear();
    queueL2.clear(); queueR2.clear();
    noInterrupts();
    totalSamplesWritten = 0;
    interrupts();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Saved");
    display.display();
    delay(2000);
    drawUI();

    Serial.print("Total samples per channel: ");
    Serial.println(sampleCount);
  }
}