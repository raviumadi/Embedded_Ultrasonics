/* ESPERDYNE
================================================================================
Project: ESP32 Heterodyne Monitor + WAV Playback (192 kHz, Mono)
Author : Ravi Umadi
Target : ESP32 (Arduino core + ESP-IDF I²S driver)
Date   : 2025-08-02

OVERVIEW
--------
This sketch turns an ESP32 into a simple ultrasonic tool that can:
  1) Heterodyne an external ultrasonic input (I²S RX) down to audible in real time.
  2) Play a 16-bit PCM mono WAV file from microSD via I²S TX when a button is pressed.

Operation is “monitor by default”: the device continuously reads from the I²S input,
multiplies the signal by a 38 kHz cosine (down-conversion), and sends the result to
I²S OUT. When the button is pressed, it temporarily plays the first WAV file it finds
on the SD card; on completion, it returns to live heterodyne monitoring.

ARCHITECTURE
------------
- Sample rate: 192000 Hz (mono, 16-bit).
- I²S Input  (I2S_NUM_0): RX, LEFT-only slot (mono).
- I²S Output (I2S_NUM_1): TX, LEFT-only slot (mono).
- Clocking: APLL enabled, MCLK fixed at 256 × Fs for clean 192 kHz.
- SD: Arduino SD library; first *.wav in root is used for playback (header skipped).

HETERODYNE PATH
---------------
- Local oscillator: freqShiftHz = 38 kHz (set in code).
- Implementation: y[n] = x[n] * cos(2π·freqShiftHz·n/Fs).
- Additional fixed gain (×10) then hard-clipped to int16. Adjust as needed to
  avoid distortion or hearing discomfort.

PLAYBACK PATH
-------------
- On button press (active-LOW), playback mode starts:
  • Opens the first *.wav in SD root (ignores resource files like “._name.wav”).
  • Skips 44-byte WAV header (assumes PCM 16-bit mono at the configured Fs).
  • Streams file bytes to I²S OUT until EOF, then rewinds to 44 and returns to monitor.
- If your WAV files use a different format (channels/rate/bit depth), convert them
  or adapt the code to parse the header and reconfigure I²S dynamically.

PINS & WIRING (DEFAULTS IN THIS SKETCH)
---------------------------------------
I²S INPUT (I2S_NUM_0, microphone / ADC):
  - BCK  -> GPIO 32  (#define I2S_IN_BCK_IO)
  - WS   -> GPIO 33  (#define I2S_IN_WS_IO)
  - DATA -> GPIO 35  (#define I2S_IN_DI_IO)

I²S OUTPUT (I2S_NUM_1, DAC / amplifier):
  - BCK  -> GPIO 25  (#define I2S_OUT_BCK_IO)
  - WS   -> GPIO 27  (#define I2S_OUT_WS_IO)
  - DATA -> GPIO 26  (#define I2S_OUT_DO_IO)

CONTROL:
  - BUTTON_PIN -> GPIO 4 (active-LOW, INPUT_PULLUP). Press to play the WAV.

SD CARD:
  - Uses Arduino SD.begin() (board default CS). If your SD module uses a custom CS,
    call SD.begin(<CS_PIN>) instead.

CONFIGURATION KNOBS
-------------------
#define SAMPLE_RATE        192000   // I²S RX/TX sample rate (Hz)
#define BITS_PER_SAMPLE    I2S_BITS_PER_SAMPLE_16BIT
const float freqShiftHz   = 38000.0f;   // Heterodyne LO (Hz)
const float angularFreq   = 2π·freqShiftHz/SAMPLE_RATE

I²S DMA buffers (both RX and TX):
  - dma_buf_count = 8
  - dma_buf_len   = 512   // frames per DMA buffer

BUILD REQUIREMENTS
------------------
- Arduino core for ESP32 (Board Manager).
- Libraries: SD, SPI (bundled), and ESP-IDF I²S driver via "driver/i2s.h".
- APLL must be available (most ESP32 variants). Keep wiring short and clean for I²S.

QUICK START
-----------
1) Wire I²S IN (mic/ADC) and I²S OUT (DAC/amp) to the pins above.
2) Insert an SD card with at least one PCM 16-bit mono WAV at 192 kHz in the root.
3) Flash the sketch and open Serial Monitor @ 115200.
4) On boot you should see the actual I²S sample rate printed.
5) You should hear live heterodyned audio. Press the button to play the WAV.

TROUBLESHOOTING
---------------
- “SD card failed”: check wiring and CS; try SD.begin(CS).
- Silence on output: verify the DAC path, I²S pin mapping, and amplifier/speaker.
- Distortion/clipping: reduce front-end gain or the ×10 gain in the loop.
- Wrong playback speed/pitch: ensure your WAV truly matches 192 kHz, 16-bit mono.
- No input: confirm your microphone/ADC outputs I²S (standard MSB-justified I²S).

SAFETY & ETHICS
---------------
- Ultrasonic emission and field playback may require permits and ethical approval
  depending on local regulations and species protection laws. Use responsibly.
- Protect your hearing; output gain can be high.

DISCLAIMER
----------
This code is provided **“as is”** without any warranty, express or implied, including
but not limited to fitness for a particular purpose. You are responsible for testing
and validating the code and hardware in your application, especially in field work
and wildlife contexts. Use at your own risk.

(Optionally pair this with your chosen licence, e.g., CC BY-NC 4.0.)
================================================================================
*/
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "driver/i2s.h"
#include <math.h>

// === CONFIGURATION ===
#define SAMPLE_RATE 192000
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define CHANNELS 1

// === I2S PORTS ===
#define I2S_IN_PORT I2S_NUM_0
#define I2S_OUT_PORT I2S_NUM_1

// === I2S PINS ===
#define I2S_IN_BCK_IO 32
#define I2S_IN_WS_IO 33
#define I2S_IN_DI_IO 35
#define I2S_OUT_BCK_IO 25
#define I2S_OUT_WS_IO 27
#define I2S_OUT_DO_IO 26

#define BUTTON_PIN 4

// === HETERODYNE PARAMETERS ===
const float freqShiftHz = 38000.0f;
const float angularFreq = 2.0f * M_PI * freqShiftHz / SAMPLE_RATE;
float phase = 0.0f;

File audioFile;
bool isPlaying = false;

void setupI2SInput() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = true,
    .tx_desc_auto_clear = false,
    .fixed_mclk = SAMPLE_RATE * 256
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
    .dma_buf_count = 8,
    .dma_buf_len = 512,
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

bool skipWavHeader(File& file) {
  if (file && file.size() > 44) {
    file.seek(44);
    return true;
  }
  return false;
}

void findAndOpenWavFile() {
  File root = SD.open("/");
  while (true) {
    File file = root.openNextFile();
    if (!file) break;
    String fname = String(file.name());
    if (!file.isDirectory() && fname.endsWith(".wav") && !fname.startsWith("._")) {
      Serial.println("Using file: " + fname);
      audioFile = file;
      break;
    }
    file.close();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  if (!SD.begin()) {
    Serial.println("SD card failed");
    while (1)
      ;
  }
  findAndOpenWavFile();
  if (!audioFile || !skipWavHeader(audioFile)) {
    Serial.println("No valid WAV file");
    while (1)
      ;
  }
  setupI2SInput();
  setupI2SOutput();
  float rate = i2s_get_clk(I2S_OUT_PORT);
  Serial.printf("I2S clock actual sample rate: %.2f Hz\n", rate);
  Serial.println("System ready");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    isPlaying = true;
    Serial.println("Button pressed: playing WAV");
  }

  if (isPlaying) {
    uint8_t buf[1024];
    size_t bytesRead, bytesWritten;
    if (!audioFile.available()) {
      Serial.println("Playback done");
      audioFile.seek(44);  // rewind
      isPlaying = false;
      delay(500);
      return;
    }
    bytesRead = audioFile.read(buf, sizeof(buf));
    if (bytesRead > 0) {
      i2s_write(I2S_OUT_PORT, buf, bytesRead, &bytesWritten, portMAX_DELAY);
    }
  } else {
    int16_t buffer[256];
    size_t bytesRead = 0, bytesWritten = 0;
    i2s_read(I2S_IN_PORT, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
    size_t samplesRead = bytesRead / sizeof(int16_t);
    for (size_t i = 0; i < samplesRead; i++) {
      float s = (float)(buffer[i]) / 32768.0f;
      s *= 10.0f;  // gain
      s *= cosf(phase);
      phase += angularFreq;
      if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
      s = constrain(s, -1.0f, 1.0f);
      buffer[i] = (int16_t)(s * 32767.0f);
    }
    i2s_write(I2S_OUT_PORT, buffer, samplesRead * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
  }
}
