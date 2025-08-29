/*
  ==============================================================================
   ESPERDYNE – Ring Buffer and Dual Channel Heterodyne Edition (v1.1)
   ================================================================================
   Author  : Ravi Umadi
   License : Open Source (CC-BY-SA 4.0)
   Date    : August 2025
   Target  : ESP32-S3-based custom board
   MCU     : ESP32-S3 (8MB PSRAM, 16MB Flash)
   Display : 128x32 OLED (SSD1306 I2C)
   Storage : microSD (SPI)
   Audio   : Dual-channel 192 kHz I2S input and output
   =============================================================================
   DESCRIPTION
   ------------------------------------------------------------------------------
   ESPERDYNE is a high-fidelity, real-time heterodyne bat call listener and 
   recorder designed for field use. It enables:
   - Real-time dual-channel heterodyne audio output at 192 kHz sample rate.
   - Adjustable carrier frequency and output volume.
   - Independent frequency control per channel.
   - Tap-to-save feature: saves the most recent audio from a 5-second ring buffer.
   - Simple UI via 128x32 OLED with file index, frequencies, volume, mode.
   - Rotary encoder and buttons for intuitive control.
   - Toggleable stereo/mix mode for playback.

   This project is intended for scientific fieldwork and behavioural monitoring 
   of echolocating animals (e.g. bats), especially in budget-limited contexts.

   ------------------------------------------------------------------------------
   FEATURES
   ------------------------------------------------------------------------------
   - Dual I2S support (I2S0 = input, I2S1 = output)
   - Stereo heterodyne demodulation with independent carrier frequencies
   - OLED UI with file number, frequency display, gain, mode, and mixing status
   - 5-second ring buffer stored in PSRAM
   - Tap-to-save recording writes ring buffer to a WAV file on SD
   - Mixing mode: toggles stereo or mono output of both channels
   - Volume and frequency control using rotary encoder
   - Startup file index scanning to avoid overwrites (REC000.wav → REC999.wav)

   ------------------------------------------------------------------------------
   UI CONTROLS
   ------------------------------------------------------------------------------
   - Rotary Encoder (CLK/DT): Adjust frequency or volume
   - Encoder Button: Toggle between "F" (frequency) and "V" (volume) mode
   - CH2 Adjust Button: Hold to adjust channel 2 frequency instead of channel 1
   - Record Button: Tap to save the last 5 seconds into a WAV file
   - OLED Toggle Button: Long press (0.5s) to turn OLED display on/off
   - Mix Toggle Button: Long press (0.5s) to toggle stereo mix mode

   ------------------------------------------------------------------------------
   OLED DISPLAY OVERVIEW (128x32)
   ------------------------------------------------------------------------------
     kHz     Mode    %     #   kHz
     XX       F     YY    ##   XX
     [MX]                            <- Bottom-left corner shows MIX/ST mode
     ESPERDYNE                       <- Center title

   F = Frequency control mode (default)
   V = Volume control mode
   MX = Mix mode enabled (both channels averaged and played in mono)
   ST = Stereo mode (independent channels)
   #  = Current file index for naming saved WAV files (e.g., REC005.WAV)

   ------------------------------------------------------------------------------
   PINOUT SUMMARY
   ------------------------------------------------------------------------------
   INPUT / OUTPUT
   -------------------------
   RECORD_BUTTON_PIN     → Pin 7   (Tap to save audio)
   CH2_HET_BUTTON_PIN    → Pin 1   (Hold to edit channel 2 frequency)
   OLED_TOGGLE_PIN       → Pin 3   (Hold to toggle OLED screen)
   MIX_TOGGLE_PIN        → Pin 2   (Hold to toggle MIX/ST mode)

   ROTARY ENCODER
   -------------------------
   ENCODER_A_PIN         → Pin 6
   ENCODER_B_PIN         → Pin 5
   ENCODER_SW_PIN        → Pin 4   (Press to toggle F/V mode)

   OLED DISPLAY (SSD1306)
   -------------------------
   I2C (default):
   SDA                   → ESP32 default (GPIO 8 or 21)
   SCL                   → ESP32 default (GPIO 9 or 22)

   SD CARD
   -------------------------
   SD_CS_PIN             → Pin 10
   SPI_MOSI              → Pin 11
   SPI_MISO              → Pin 13
   SPI_SCK               → Pin 12

   I2S AUDIO INPUT (I2S0)
   -------------------------
   I2S_IN_BCK_IO         → GPIO 39
   I2S_IN_WS_IO          → GPIO 41
   I2S_IN_DI_IO          → GPIO 40
   I2S_IN_MCK_IO         → GPIO 42

   I2S AUDIO OUTPUT (I2S1)
   -------------------------
   I2S_OUT_BCK_IO        → GPIO 15
   I2S_OUT_WS_IO         → GPIO 17
   I2S_OUT_DO_IO         → GPIO 16

   ------------------------------------------------------------------------------
   WAV FILE FORMAT
   ------------------------------------------------------------------------------
   - Stereo (2 channel)
   - 16-bit PCM
   - 192000 samples/sec
   - Filename: REC###.WAV (auto-incrementing)
   - Stored on SD card using SdFat library (high-speed, SPI mode)

   ------------------------------------------------------------------------------
   MEMORY USAGE
   ------------------------------------------------------------------------------
   - 5 seconds stereo @ 192kHz 16-bit = 192000 * 2 * 5 * 2 = 3.84 MB
   - Ring buffer stored in PSRAM
   - Temporary buffers use internal RAM
   - Recording is blocked during file writing to avoid underrun

   ------------------------------------------------------------------------------
   FUTURE IMPROVEMENTS (Optional)
   ------------------------------------------------------------------------------
   - Adjustable ring buffer length (e.g., via menu or compile-time define)
   - OLED menu system for file browser, frequency presets, etc.
   - Real-time spectrogram display (Using larger TFT Display)
   - Optional pre-trigger and post-trigger logic
  
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SdFat.h>
#include <math.h>
#include "driver/i2s.h"
#include "esp_heap_caps.h"
#include <Fonts/FreeSerif9pt7b.h>        // for serif font
#include <Fonts/FreeSerifItalic9pt7b.h>  // for italic version

#define SAMPLE_RATE 192000
#define CHANNELS 2
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define BUFFER_SECONDS 5
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

#define RECORD_BUTTON_PIN 7
#define CH2_HET_BUTTON_PIN 1
#define OLED_TOGGLE_PIN 3
#define MIX_TOGGLE_PIN 2
#define ENCODER_A_PIN 6
#define ENCODER_B_PIN 5
#define ENCODER_SW_PIN 4
#define SD_CS_PIN 10
#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_SCK 12
#define SPI_SPEED SD_SCK_MHZ(25)

#define I2S_IN_PORT I2S_NUM_0
#define I2S_OUT_PORT I2S_NUM_1
#define I2S_IN_BCK_IO 39
#define I2S_IN_WS_IO 41
#define I2S_IN_DI_IO 40
#define I2S_IN_MCK_IO 42
#define I2S_OUT_BCK_IO 15
#define I2S_OUT_WS_IO 17
#define I2S_OUT_DO_IO 16

float carrierFreq1 = 45000.0f;
float carrierFreq2 = 45000.0f;
float outGain = 0.5f;
float phase1 = 0.0f, phase2 = 0.0f;
const float angularFreqBase = 2.0f * M_PI / SAMPLE_RATE;
float angularFreq1 = angularFreqBase * carrierFreq1;
float angularFreq2 = angularFreqBase * carrierFreq2;
bool mixingMode = true;  // true = Mix ON (default), false = Mix OFF
unsigned long lastMixButtonTime = 0;

#define RING_BUFFER_SIZE (SAMPLE_RATE * CHANNELS * BUFFER_SECONDS)
int16_t* ringBuffer;
volatile size_t writeIndex = 0;
size_t ringFill = 0;
int fileIndex = 0;

int encoderTicks = 0;
uint8_t lastEncState = 0;
const int TICKS_PER_DETENT = 4;

enum EditMode { EDIT_FREQ = 0,
                EDIT_VOL = 1 };
EditMode editMode = EDIT_FREQ;

const float FREQ_MIN = 10000.0f, FREQ_MAX = 85000.0f, FREQ_STEP = 5000.0f;
const float VOL_STEP = 0.05f;

bool saving = false;
bool oledOn = true;
unsigned long lastOLEDButtonTime = 0;
bool ch2AdjustMode = false;

SdFat sd;
SdFile file;

void writeWavHeader(SdFile& file, uint32_t sampleRate, uint16_t bitsPerSample, uint32_t numSamples) {
  uint32_t dataChunkSize = numSamples * bitsPerSample / 8;
  uint32_t chunkSize = 36 + dataChunkSize;
  file.seekSet(0);
  file.write((const uint8_t*)"RIFF", 4);
  file.write((uint8_t*)&chunkSize, 4);
  file.write((const uint8_t*)"WAVE", 4);
  file.write((const uint8_t*)"fmt ", 4);
  ;
  uint32_t subchunk1Size = 16;
  uint16_t audioFormat = 1;
  uint16_t numChannels = CHANNELS;
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint16_t blockAlign = numChannels * bitsPerSample / 8;
  file.write((uint8_t*)&subchunk1Size, 4);
  file.write((uint8_t*)&audioFormat, 2);
  file.write((uint8_t*)&numChannels, 2);
  file.write((uint8_t*)&sampleRate, 4);
  file.write((uint8_t*)&byteRate, 4);
  file.write((uint8_t*)&blockAlign, 2);
  file.write((uint8_t*)&bitsPerSample, 2);
  file.write((const uint8_t*)"data", 4);
  file.write((uint8_t*)&dataChunkSize, 4);
}

String getNextFilename() {
  char fname[20];
  while (true) {
    snprintf(fname, sizeof(fname), "/REC%03d.WAV", fileIndex++);
    if (!sd.exists(fname)) return String(fname);
  }
}

void drawUI() {
  if (!oledOn) return;
  display.clearDisplay();

  // --- Title: ESPERDYNE ---
  display.setFont();
  display.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("ESPERDYNE", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, 25);  // centered
  display.print("ESPERDYNE");

  // display.setCursor(0, 25);
  // display.print("MX");  // static label

  display.setCursor(0, 0);
  display.print("kHz");

  display.setCursor(28, 0);
  display.print("Mode");

  display.setCursor(60, 0);
  display.print("%");

  display.setCursor(110, 0);
  display.print("kHz");

  // --- File index label ---
  display.setCursor(85, 0);
  display.print("#");

  // --- Frequencies (normal serif) ---
  display.setFont(&FreeSerif9pt7b);
  display.setCursor(0, 21);
  display.print(int(carrierFreq1 / 1000.0f));
  display.setCursor(110, 21);
  display.print(int(carrierFreq2 / 1000.0f));

  // --- Mode (F or V) ---
  display.setCursor(31, 21);
  display.print(editMode == EDIT_FREQ ? "F" : "V");

  // --- Volume ---
  display.setCursor(53, 21);
  display.print((int)(outGain * 100));

  // --- File index ---
  display.setCursor(82, 21);
  display.print(fileIndex);

  // --- Mixing Mode Status: "MIX" or "ST" ---
  display.setFont();  // default font
  display.setTextSize(1);
  display.setCursor(0, OLED_HEIGHT - 8);  // bottom-left corner
  display.print(mixingMode ? "MX" : "ST");

  display.display();
}

void splashScreen() {
  display.clearDisplay();
  display.setFont(&FreeSerif9pt7b);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("ESPERDYNE", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, 12);
  display.print("ESPERDYNE");

  display.getTextBounds("Ravi Umadi", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, 28);
  display.print("Ravi Umadi");

  display.display();
  delay(2000);
}

void setupI2SInput() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 10,
    .dma_buf_len = 1024,
    .use_apll = true,
    .tx_desc_auto_clear = false,
    .fixed_mclk = SAMPLE_RATE * 256
  };
  i2s_pin_config_t pins = {
    .mck_io_num = I2S_IN_MCK_IO,
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
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
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

void saveFromRingBuffer() {
  size_t samplesToWrite = min(ringFill, static_cast<size_t>(RING_BUFFER_SIZE));
  if (samplesToWrite == 0) return;
  String fname = getNextFilename();
  if (!file.open(fname.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) return;
  size_t startIndex = (writeIndex + RING_BUFFER_SIZE - samplesToWrite) % RING_BUFFER_SIZE;
  writeWavHeader(file, SAMPLE_RATE, 16, samplesToWrite);
  const size_t blockSize = 1024 * 4;
  static int16_t tempBlock[blockSize];
  for (size_t i = 0; i < samplesToWrite; i += blockSize) {
    size_t actualBlock = min(blockSize, samplesToWrite - i);
    for (size_t j = 0; j < actualBlock; ++j) {
      size_t idx = (startIndex + i + j) % RING_BUFFER_SIZE;
      tempBlock[j] = ringBuffer[idx];
    }
    file.write((uint8_t*)tempBlock, actualBlock * sizeof(int16_t));
  }
  file.close();
  drawUI();
}

void setup() {
  Serial.begin(115200);
  pinMode(RECORD_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CH2_HET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OLED_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(MIX_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setRotation(2);
  display.setTextColor(WHITE);
  splashScreen();

  if (!sd.begin(SD_CS_PIN, SPI_SPEED)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("SD FAIL");
    display.display();
    while (1)
      ;
  }

  // === Count existing REC###.WAV files ===
  SdFile dir;
  if (!dir.open("/")) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("DIR OPEN FAIL");
    display.display();
    while (1);
  }

  char fname[32];
  SdFile entry;
  while (entry.openNext(&dir, O_RDONLY)) {
    if (entry.isFile()) {
      entry.getName(fname, sizeof(fname));
      if (strncmp(fname, "REC", 3) == 0 && strstr(fname, ".WAV")) {
        int index = atoi(&fname[3]);  // read the 3 digits after "REC"
        if (index >= fileIndex) {
          fileIndex = index + 1;
        }
      }
    }
    entry.close();
  }
  dir.close();

  ringBuffer = (int16_t*)ps_malloc(RING_BUFFER_SIZE * sizeof(int16_t));
  delay(100); // Keep this delay to avoid racing. Increase if necessary

  if (!ringBuffer) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("RAM FAIL");
    display.display();
    while (1)
      ;
  }

  drawUI();

  uint8_t a = digitalRead(ENCODER_A_PIN);
  uint8_t b = digitalRead(ENCODER_B_PIN);
  lastEncState = (a << 1) | b;

  setupI2SInput();
  setupI2SOutput();
  delay(1000);
}

void loop() {
  if (digitalRead(OLED_TOGGLE_PIN) == LOW && millis() - lastOLEDButtonTime > 500) {
    oledOn = !oledOn;
    lastOLEDButtonTime = millis();

    if (!oledOn) {
      display.clearDisplay();
      display.display();  // force clear
      display.ssd1306_command(SSD1306_DISPLAYOFF);
    } else {
      display.ssd1306_command(SSD1306_DISPLAYON);
      drawUI();  // redraw interface
    }
  }
  if (digitalRead(MIX_TOGGLE_PIN) == LOW && millis() - lastMixButtonTime > 500) {
  mixingMode = !mixingMode;
  lastMixButtonTime = millis();
  drawUI();  // optionally update the screen to show mode
}

  ch2AdjustMode = (digitalRead(CH2_HET_BUTTON_PIN) == LOW);

  const int bufferSize = 1024;
  int16_t buffer[bufferSize], playBuffer[bufferSize];
  size_t bytesRead = 0, bytesWritten = 0;
  i2s_read(I2S_IN_PORT, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
  size_t samplesRead = bytesRead / sizeof(int16_t);

  for (size_t i = 0; i < samplesRead; i++) {
    ringBuffer[writeIndex] = buffer[i];
    writeIndex = (writeIndex + 1) % RING_BUFFER_SIZE;
    if (ringFill < RING_BUFFER_SIZE) ringFill++;
  }

  for (size_t i = 0; i < samplesRead; i += 2) {
  float s1 = buffer[i] / 32768.0f * 10.0f * cosf(phase1);
  phase1 += angularFreq1;
  if (phase1 > 2.0f * M_PI) phase1 -= 2.0f * M_PI;
  s1 = constrain(s1 * outGain, -1.0f, 1.0f);

  float s2 = buffer[i + 1] / 32768.0f * 10.0f * cosf(phase2);
  phase2 += angularFreq2;
  if (phase2 > 2.0f * M_PI) phase2 -= 2.0f * M_PI;
  s2 = constrain(s2 * outGain, -1.0f, 1.0f);

  if (mixingMode) {
    float avg = (s1 + s2) * 0.5f;
    int16_t mixed = (int16_t)(constrain(avg, -1.0f, 1.0f) * 32767.0f);
    playBuffer[i] = mixed;
    playBuffer[i + 1] = mixed;
  } else {
    playBuffer[i] = (int16_t)(s1 * 32767.0f);
    playBuffer[i + 1] = (int16_t)(s2 * 32767.0f);
  }
}

  i2s_write(I2S_OUT_PORT, playBuffer, samplesRead * sizeof(int16_t), &bytesWritten, portMAX_DELAY);

  if (digitalRead(RECORD_BUTTON_PIN) == LOW && !saving) {
    saving = true;
    display.clearDisplay();
    display.setFont(&FreeSerif9pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds("Saving File", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_WIDTH - w) / 2, 16);  // centered
    display.print("Saving File");
    display.display();
    i2s_stop(I2S_IN_PORT);
    saveFromRingBuffer();
    i2s_start(I2S_IN_PORT);
    saving = false;
    drawUI();
  }

  uint8_t a = digitalRead(ENCODER_A_PIN), b = digitalRead(ENCODER_B_PIN);
  uint8_t state = (a << 1) | b;
  static const int8_t enc_table[] = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };
  int8_t movement = enc_table[(lastEncState << 2) | state];
  lastEncState = state;
  if (movement != 0) {
    encoderTicks += movement;
    int detents = -1 * encoderTicks / TICKS_PER_DETENT;
    encoderTicks %= TICKS_PER_DETENT;
    if (detents != 0) {
      if (editMode == EDIT_FREQ) {
        if (ch2AdjustMode) {
          carrierFreq2 = constrain(carrierFreq2 + detents * FREQ_STEP, FREQ_MIN, FREQ_MAX);
          angularFreq2 = angularFreqBase * carrierFreq2;
        } else {
          carrierFreq1 = constrain(carrierFreq1 + detents * FREQ_STEP, FREQ_MIN, FREQ_MAX);
          angularFreq1 = angularFreqBase * carrierFreq1;
        }
      } else {
        outGain = constrain(outGain + detents * VOL_STEP, 0.0f, 1.0f);
      }
      drawUI();
    }
  }

  if (digitalRead(ENCODER_SW_PIN) == LOW) {
    delay(200);
    editMode = (editMode == EDIT_FREQ) ? EDIT_VOL : EDIT_FREQ;
    drawUI();
  }
}