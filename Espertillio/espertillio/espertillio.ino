/* ESPERTILLIO
===============================================================================
Project: ESP32 Ultrasonic Heterodyne + Bat Call Player/Recorder (192 kHz, mono)
Author : Ravi Umadi
Target : ESP32 (Arduino core)
Date   : 2025-08-02

OVERVIEW
--------
This sketch turns an ESP32 into a simple ultrasonic tool that can:
  1) Heterodyne an external ultrasonic microphone input to audible output.
  2) Generate and play a synthetic bat call (FM or CF) to the I²S output.
  3) Record raw mono input to WAV on microSD at 192 kHz / 16-bit.

It uses two I²S peripherals simultaneously:
  - I2S_NUM_0 as INPUT (RX)  @ 192 kHz, 16-bit, mono (left slot).
  - I2S_NUM_1 as OUTPUT (TX) @ 192 kHz, 16-bit, mono (left slot).
A microSD card (SPI) is used for logging WAV files named /REC###.WAV.

CONTROLS
--------
- BUTTON_PIN (GPIO 4): Press to PLAY a generated bat call.
  * SWITCH_PIN LOW  -> Play CF (constant-frequency) call (Doppler option included).
  * SWITCH_PIN HIGH -> Play FM (linear downward chirp) call.

- RECORD_BUTTON (GPIO 2):
  * LOW  -> Start recording (creates /REC###.WAV and reserves header).
  * HIGH -> Stop recording (WAV header is finalised and file is closed).

- SWITCH_PIN (GPIO 16): Selects CF vs FM for the playback button (see above).

SIGNAL FLOW
-----------
Input path (heterodyne monitor):
  I²S Mic/ADC -> I2S_NUM_0 -> 16-bit buffer -> multiply by cos(2π·38 kHz·t)
  -> scaling/clip -> I2S_NUM_1 -> I²S DAC/Codec -> Speaker/Headphones

The local oscillator is fixed at 38 kHz (freqShiftHz). Multiplication by a cosine.
shifts spectral content down by 38 kHz (classic heterodyne), so ultrasonic calls
within ~35–45 kHz become audible. You can change freqShiftHz to tune for other bands.

Recording path:
  I²S input -> ring buffer (RAM) -> writes chunks to SD (SPI) -> WAV header finalised on stop.

FILES & FORMATS
---------------
- WAV: PCM, 16-bit, mono, 192,000 samples/s.
- Filenames auto-increment: /REC000.WAV, /REC001.WAV, ...
- WAV header is written/overwritten correctly on stop (data size is accurate).

TIMING & BUFFERS
----------------
- SAMPLE_RATE:     192000 Hz (16-bit mono).
- I²S DMA buffers: 10 buffers × 1024 samples (both RX and TX).
- Ring buffer for SD writes: RING_BUFFER_SIZE bytes (default 8192).
  Increase if your SD card is slow; too small may cause dropouts during recording.

BAT CALL SYNTHESIS
------------------
- FM call  : Linear chirp from CALL_F0_HZ (default 40 kHz) down to CALL_F1_HZ (default 5 kHz),
             duration CALL_DURATION_MS, with Hann windowing and optional symmetric tail padding.
- CF call  : Single-frequency tone at CALL_F0_HZ with Hann window; optional Doppler resampling
             using DOPPLER_VELOCITY (m/s) to simulate source/receiver motion.
- CALL_VOLUME_SCALE: Scales the generated call amplitude before converting to int16.

PINOUT (ESP32)
--------------
I²S INPUT  (I2S_NUM_0):
  - BCK  -> GPIO 32  (I2S_IN_BCK_IO)
  - WS   -> GPIO 33  (I2S_IN_WS_IO)
  - DATA -> GPIO 35  (I2S_IN_DI_IO)

I²S OUTPUT (I2S_NUM_1):
  - BCK  -> GPIO 25  (I2S_OUT_BCK_IO)
  - WS   -> GPIO 27  (I2S_OUT_WS_IO)
  - DATA -> GPIO 26  (I2S_OUT_DO_IO)

SD CARD (SPI mode):
  - CS   -> GPIO 5   (SD_CS_PIN)
  - SCK/MOSI/MISO -> board defaults for VSPI (GPIO 18 / 23 / 19 on many boards)

BUTTONS:
  - BUTTON_PIN     -> GPIO 4  (play call, active LOW, internal pull-up)
  - RECORD_BUTTON  -> GPIO 2  (start/stop, simple level check)
  - SWITCH_PIN     -> GPIO 16 (CF/FM select, read level)

BUILD REQUIREMENTS
------------------
- Arduino core for ESP32 (ESP-IDF underneath).
- SD library (Arduino) and SPI.
- Uses ESP32 I²S driver via "driver/i2s.h".
- APLL enabled for precise 192 kHz clocking (fixed MCLK = 256×Fs).

KEY CONSTANTS TO TUNE
---------------------
- SAMPLE_RATE     : 192000 (supported by your ADC/DAC chain)
- freqShiftHz     : Heterodyne LO (default 38 kHz)
- CALL_F0_HZ/F1_HZ: Bat call frequencies
- CALL_DURATION_MS: Bat call length (ms)
- CALL_TAIL_PCT   : Zero padding on both sides (for neat windowed placement)
- DOPPLER_VELOCITY: m/s (CF call Doppler simulation)
- RING_BUFFER_SIZE: SD write chunking (increase for safer writes)

PERFORMANCE NOTES
-----------------
- The main loop uses blocking I²S reads/writes with portMAX_DELAY for simplicity.
- During recording, audio is buffered to RAM and flushed when the ring buffer fills.
  If your SD is slow, increase RING_BUFFER_SIZE or use faster cards.
- Live heterodyne path applies a gain of 10.0f before limiting; adjust if clipping.

SAFETY & LIMITATIONS
--------------------
- No formal button debouncing; simple level checks with small delays on start/stop.
- Generators allocate temporary arrays with new/delete; ensure heap is sufficient.
- Ensure analogue front-end (mic preamp / ADC) and output stage are wired correctly.
- 192 kHz at 16-bit mono is demanding; power the board and SD card with a stable supply.

QUICK START
-----------
1) Wire I²S IN (mic/ADC), I²S OUT (DAC/amp), and microSD as above.
2) Insert formatted microSD.
3) Flash this sketch. Open Serial Monitor (115200).
4) Press RECORD_BUTTON LOW to start recording, HIGH to stop and save WAV.
5) Press BUTTON_PIN to play a generated call (CF vs FM via SWITCH_PIN).
6) For live listening, feed ultrasonic signal to I²S IN; heterodyned audio appears on I²S OUT.

EXTENSIONS (IDEAS)
------------------
- Make freqShiftHz adjustable via a rotary encoder or serial commands.
- Add on-screen/OLED status and metering (peak, clip indicator).
===============================================================================

Disclaimer

This code is provided **"as is"** without any warranties, express or implied.  
The author makes no guarantee regarding the correctness, safety, or fitness of this code for any particular purpose.  
You are responsible for reviewing, testing, and validating the code before using it in any application, especially those involving safety-critical or commercial systems.

By using this code, you agree that:

- The author is **not liable** for any direct, indirect, incidental, or consequential damages resulting from its use.
- The code is intended for **educational and research purposes only**.
- It must not be used in violation of any applicable laws or regulations.

Use at your own risk.

License

This work is licensed under a  
[Creative Commons Attribution–NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).
*/

#include <Arduino.h>
#include "driver/i2s.h"
#include <math.h>
#include <SD.h>
#include <SPI.h>

// === CONFIGURATION ===
#define SAMPLE_RATE 192000
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define CHANNELS 1
#define BUTTON_PIN 4
#define RECORD_BUTTON 2
#define SWITCH_PIN 16
#define SD_CS_PIN 5

#define I2S_IN_PORT I2S_NUM_0
#define I2S_OUT_PORT I2S_NUM_1

#define I2S_IN_BCK_IO 32
#define I2S_IN_WS_IO 33
#define I2S_IN_DI_IO 35
#define I2S_OUT_BCK_IO 25
#define I2S_OUT_WS_IO 27
#define I2S_OUT_DO_IO 26

const float freqShiftHz = 38000.0f;
const float angularFreq = 2.0f * M_PI * freqShiftHz / SAMPLE_RATE;
float phase = 0.0f;

#define CALL_F0_HZ 40000.0f
#define CALL_F1_HZ 5000.0f
#define CALL_DURATION_MS 50.0f
#define CALL_TAIL_PCT 50
#define DOPPLER_VELOCITY 0.0f
const float CALL_VOLUME_SCALE = 0.1f;

const int callSamples = int(CALL_DURATION_MS / 1000.0f * SAMPLE_RATE);
const int tailSamples = int((CALL_TAIL_PCT / 100.0f) * callSamples);
const int totalSamples = callSamples + 2 * tailSamples;
int16_t batCall[totalSamples * 2];

bool isPlaying = false;
bool isRecording = false;
int fileIndex = 0;
File recordingFile;

#define RING_BUFFER_SIZE 8192  // Tune this depending on available RAM
uint8_t ringBuffer[RING_BUFFER_SIZE];
size_t ringIndex = 0;

// === WAV HEADER ===
void writeWavHeader(File& file, uint32_t sampleRate, uint16_t bitsPerSample, uint32_t numSamples) {
  uint32_t dataChunkSize = numSamples * bitsPerSample / 8;
  uint32_t chunkSize = 36 + dataChunkSize;
  file.seek(0);
  file.write(reinterpret_cast<const uint8_t*>("RIFF"), 4);
  file.write((uint8_t*)&chunkSize, 4);
  file.write(reinterpret_cast<const uint8_t*>("WAVE"), 4);
  file.write(reinterpret_cast<const uint8_t*>("fmt "), 4);

  uint32_t subchunk1Size = 16;
  uint16_t audioFormat = 1;
  uint16_t numChannels = 1;
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint16_t blockAlign = numChannels * bitsPerSample / 8;

  file.write((uint8_t*)&subchunk1Size, 4);
  file.write((uint8_t*)&audioFormat, 2);
  file.write((uint8_t*)&numChannels, 2);
  file.write((uint8_t*)&sampleRate, 4);
  file.write((uint8_t*)&byteRate, 4);
  file.write((uint8_t*)&blockAlign, 2);
  file.write((uint8_t*)&bitsPerSample, 2);
  file.write(reinterpret_cast<const uint8_t*>("data"), 4);
  file.write((uint8_t*)&dataChunkSize, 4);
}

String getNextFilename() {
  char fname[20];
  while (true) {
    snprintf(fname, sizeof(fname), "/REC%03d.WAV", fileIndex++);
    if (!SD.exists(fname)) return String(fname);
  }
}

void startRecording() {
  String fname = getNextFilename();
  recordingFile = SD.open(fname.c_str(), FILE_WRITE);
  if (!recordingFile) {
    Serial.println("Failed to open file for recording.");
    return;
  }
  // Reserve WAV header space
  for (int i = 0; i < 44; i++) recordingFile.write((uint8_t)0);
  isRecording = true;
  Serial.print("Recording to ");
  Serial.println(fname);
}

void stopRecording() {
  isRecording = false;
  
  // Flush any remaining audio in ring buffer
  if (ringIndex > 0) {
    recordingFile.write(ringBuffer, ringIndex);
    ringIndex = 0;
  }
  size_t fileSize = recordingFile.size();
  uint32_t samples = (fileSize - 44) / sizeof(int16_t);
  recordingFile.seek(0);
  writeWavHeader(recordingFile, SAMPLE_RATE, 16, samples);
  recordingFile.close();
  Serial.println("Recording stopped and saved.");
}

// === I2S SETUP ===
void setupI2SInput() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 10,
    .dma_buf_len = 1024,
    .use_apll = true,
    .tx_desc_auto_clear = false,
    .fixed_mclk = SAMPLE_RATE * 256 // <- Crucial setting for getting the I2S clock right. Without this, ESP32 defaults to 48kHz no matter what .sample_rate is set to. 
  };
  i2s_pin_config_t pins = {
    .bck_io_num = I2S_IN_BCK_IO,
    .ws_io_num = I2S_IN_WS_IO,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_IN_DI_IO
  };
  i2s_driver_install(I2S_IN_PORT, &config, 0, NULL);
  i2s_set_pin(I2S_IN_PORT, &pins);
  i2s_zero_dma_buffer(I2S_IN_PORT);
}

void setupI2SOutput() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 10,
    .dma_buf_len = 1024,
    .use_apll = true,
    .tx_desc_auto_clear = true,
    .fixed_mclk = SAMPLE_RATE * 256
  };
  i2s_pin_config_t pins = {
    .bck_io_num = I2S_OUT_BCK_IO,
    .ws_io_num = I2S_OUT_WS_IO,
    .data_out_num = I2S_OUT_DO_IO,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_OUT_PORT, &config, 0, NULL);
  i2s_set_pin(I2S_OUT_PORT, &pins);
  i2s_zero_dma_buffer(I2S_OUT_PORT);
}

// === BAT CALL GENERATION ===
void generateFMBatCall(int16_t* out) {
  for (int i = 0; i < totalSamples; i++) out[i] = 0;
  float T = CALL_DURATION_MS / 1000.0f;
  float f0 = CALL_F0_HZ, f1 = CALL_F1_HZ, k = (f1 - f0) / T;
  for (int i = 0; i < callSamples; i++) {
    float t = i / (float)SAMPLE_RATE;
    float p = 2.0f * M_PI * (f0 * t + 0.5f * k * t * t);
    float h = 0.5f * (1 - cosf(2.0f * M_PI * i / (callSamples - 1)));
    out[tailSamples + i] = (int16_t)(sinf(p) * h * 32767.0f * CALL_VOLUME_SCALE);
  }
  Serial.println("FM Bat call generated.");
}

void generateCFBatCall(float f0, float dur_ms, float fs, float tail_pct, float velocity, int16_t* out) {
  float c = 343.0f, dur_s = dur_ms / 1000.0f;
  int n = int(dur_s * fs);
  float* tone = new float[n];
  for (int i = 0; i < n; i++) tone[i] = sinf(2.0f * M_PI * f0 * i / fs) * (0.5f - 0.5f * cosf(2.0f * M_PI * i / (n - 1)));
  if (velocity != 0) {
    float doppler_ratio = sqrtf((c - velocity) / (c + velocity));
    int n_resamp = int(n * doppler_ratio);
    float* resampled = new float[n_resamp];
    for (int i = 0; i < n_resamp; i++) {
      float idx = i / doppler_ratio;
      int i0 = int(idx);
      float frac = idx - i0;
      resampled[i] = (i0 + 1 < n) ? tone[i0] * (1 - frac) + tone[i0 + 1] * frac : 0;
    }
    delete[] tone;
    tone = resampled;
    n = n_resamp;
  }
  float maxval = 0.0f;
  for (int i = 0; i < n; i++)
    if (fabsf(tone[i]) > maxval) maxval = fabsf(tone[i]);
  for (int i = 0; i < totalSamples; i++) out[i] = 0;
  for (int i = 0; i < n; i++) out[tailSamples + i] = (int16_t)(tone[i] / maxval * 32767.0f * CALL_VOLUME_SCALE);
  delete[] tone;
  Serial.println("CF Bat call generated.");
}

void writeCallToSD(const char* filename) {
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing.");
    return;
  }
  writeWavHeader(file, SAMPLE_RATE, 16, totalSamples);
  file.write((uint8_t*)batCall, totalSamples * sizeof(int16_t));
  file.close();
  Serial.println("Bat call written to SD as WAV.");
}

// === SETUP ===
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RECORD_BUTTON, INPUT_PULLUP);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD init failed!");
    return;
  }

  setupI2SInput();
  setupI2SOutput();
  Serial.println("System ready.");
}

// === LOOP ===
void loop() {
  if (digitalRead(BUTTON_PIN) == LOW && !isPlaying) {
    isPlaying = true;
    int16_t* call = new int16_t[totalSamples];
    if (digitalRead(SWITCH_PIN) == LOW)
      generateCFBatCall(CALL_F0_HZ, CALL_DURATION_MS, SAMPLE_RATE, CALL_TAIL_PCT, DOPPLER_VELOCITY, call);
    else
      generateFMBatCall(call);

    size_t bytesWritten;
    i2s_write(I2S_OUT_PORT, call, totalSamples * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    delete[] call;
    delay(500);
    isPlaying = false;
    return;
  }

  if (digitalRead(RECORD_BUTTON) == LOW && !isRecording) {
    delay(200);
    startRecording();
  }
  if (digitalRead(RECORD_BUTTON) == HIGH && isRecording) {
    delay(200);
    stopRecording();
  }

  const int bufferSize = 1024;
  int16_t buffer[bufferSize], playBuffer[bufferSize];
  size_t bytesRead = 0, bytesWritten = 0;

  i2s_read(I2S_IN_PORT, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
  size_t samplesRead = bytesRead / sizeof(int16_t);

  if (isRecording) {
    // Copy to ring buffer
    size_t spaceLeft = RING_BUFFER_SIZE - ringIndex;
    size_t copySize = min(spaceLeft, samplesRead * sizeof(int16_t));
    memcpy(&ringBuffer[ringIndex], buffer, copySize);
    ringIndex += copySize;

    // If buffer full, write to SD
    if (ringIndex >= RING_BUFFER_SIZE) {
      recordingFile.write(ringBuffer, RING_BUFFER_SIZE);
      ringIndex = 0;
    }
  } else {
    // Live heterodyne output only if not recording
    for (size_t i = 0; i < samplesRead; i++) {
      float s = buffer[i] / 32768.0f;
      s *= 10.0f * cosf(phase);
      phase += angularFreq;
      if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
      s = constrain(s, -1.0f, 1.0f);
      playBuffer[i] = (int16_t)(s * 32767.0f);
    }
    i2s_write(I2S_OUT_PORT, playBuffer, samplesRead * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
  }
}