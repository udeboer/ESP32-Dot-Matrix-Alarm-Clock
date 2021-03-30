/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// Play (wav) sounds

#include "driver/gpio.h"
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "hal/i2s_types.h"
#include "stdio.h"
#include "string.h"

// own includes below
#include "defaults_globals.h"
#include "filesystem.h"
#include "sound.h"

// Set logging tag per module
static const char *TAG = "Sound";

// Define values for play sound event.(mp3 not supported)
typedef enum {
  wav,
  mp3,
  stop_play,
} audio_file_type_t;

// Event to receive
typedef struct {
  FILE *file_desc;
  audio_file_type_t type;
  int repeat;
} file_to_play_t;

// queue for sending start play and stop events
static QueueHandle_t play_sound_queue = 0;

void i2s_play_sound();

static const int i2s_num = I2S_NUM_0;  // i2s port number

void init_i2s(void) {
  // below we startup i2s with default values for
  // samplerate and bitdepth.
  // These will be changed when we read the values
  // out of the sound file

  // Set some default values
  static const i2s_config_t i2s_config = {.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
                                          .sample_rate = 8000,
                                          .bits_per_sample = 16,
                                          .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
                                          .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                                          .dma_buf_count = 8,
                                          .dma_buf_len = 64,
                                          .use_apll = false,
                                          .intr_alloc_flags = 0,  // default interrupt priority
                                          .tx_desc_auto_clear = false,
                                          .fixed_mclk = 0};

  // Set pins used
  static const i2s_pin_config_t i2s_pin_config = {
      // see header file for pins
      .bck_io_num = I2S_BLCK_PIN,
      .ws_io_num = I2S_WS_PIN,
      .data_out_num = I2S_DATA_OUT_PIN,
      .data_in_num = I2S_PIN_NO_CHANGE  // we do not receive audio
  };

  ESP_LOGI(TAG, "i2s driver install");
  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);  // we do not use event
                                                      // queue for driver
  ESP_LOGI(TAG, "i2s pin config");
  i2s_set_pin(i2s_num, &i2s_pin_config);

#ifdef MAX98357_SD_PIN
  // When using MAX98357 and only 1 channel we can switch of DAC/AMP in
  // max98357. Pin SD low is switch off. SD high(VDD) means left channel only
  // output in max98357.
  static const gpio_config_t gpio_conf_max98357_sd = {.intr_type = GPIO_INTR_DISABLE,
                                                      .mode = GPIO_MODE_OUTPUT,
                                                      .pin_bit_mask = (1ULL << MAX98357_SD_PIN),
                                                      .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                                      .pull_up_en = GPIO_PULLUP_DISABLE};

  ESP_LOGI(TAG, "Config MAX98357 with SD pin");
  gpio_config(&gpio_conf_max98357_sd);

  ESP_LOGI(TAG, "Sleep MAX98357 with SD pin");
  // disable on startup. Gets enabled when playing sounds
  gpio_set_level(MAX98357_SD_PIN, 0);
#endif

  // startup queue and task
  ESP_LOGI(TAG, "Start play sound queue");
  play_sound_queue = xQueueCreate(8, sizeof(file_to_play_t));
  ESP_LOGI(TAG, "Start play sound task");
  xTaskCreatePinnedToCore(&i2s_play_sound, "Sound", 4096, NULL, 4, NULL, 1);
  ESP_LOGI(TAG, "Finished starting play sound task");
}

// This waits for queue messages what to play or to stop
// it receives a file descriptor, type of sound file, repeat on/off
void i2s_play_sound() {
  // Define most vars as static. Function will never return. It is a task
  static file_to_play_t file_to_play;
  static file_to_play_t peek_in_queue;
  static int size_read;
  static int size_total_read;
  static uint32_t i2s_bytes_written;
  static fpos_t data_pos;
  static int8_t play_buf[2048];  // read buffer
  // Below are structs used in various audio files
  // WAV file header
  static struct {
    uint32_t riff;  // Ascii FFIR 0x46464952 (reversed endian)
    uint32_t total_size;
    uint32_t wave;          // Ascii EVAW 0x45564157
    uint32_t SubChunkID;    // Ascii (space)tmf 0x20746d66
    uint32_t SubChunkSize;  // always 16 for PCM
    uint16_t AudioFormat;   // PCM=1
    uint16_t NumChannels;   // Mono=1 Stereo2
    uint32_t SampleRate;
    uint32_t ByteRate;       // SampleRate * Channels * Bytes per sample
    uint16_t BlockAlign;     // Channel *Bytes per sample
    uint16_t BitsPerSample;  // bits per sample 8/16/24
  } wav_file_header;

  // After Wav file header 1 or more data chunks follow. Every chunk has
  // this80123456789012345678901234567788 header. First header starts at
  // byte 36. The data follows the data chunk header at byte 44. Often only 1
  // data chunk is used
  static struct {
    uint32_t SubChunkId;    // ASCII data 0x64617461
    uint32_t SubChunkSize;  // should be equal to total samples * channels *
                            // bytes per sample
  } wav_datablock_header;

  // this task never stops running
  while (1) {
    // waiting on next file to play in queue or stop signal
    // We receive an open filedescriptor. 
    xQueueReceive(play_sound_queue, &file_to_play, portMAX_DELAY);
    // if not stop play file
    if (file_to_play.type != stop_play) {
      ESP_LOGI(TAG, "Starting to read audio file");
#ifdef MAX98357_SD_PIN
      // When using max98357 as an dac and amp get it out of sleep mode
      gpio_set_level(MAX98357_SD_PIN, 1);
#endif
      // play wav file. And when needed repeat play
      if (file_to_play.type == wav) {
        // Currently we only support wav files
        //
        // Read wav file header including fmt chunk
        fread(&wav_file_header, 1, sizeof(wav_file_header), file_to_play.file_desc);
        // do wav file check and set parameters
        ESP_LOGI(TAG, "Riff header %x", wav_file_header.riff);
        ESP_LOGI(TAG, "Total size %d", wav_file_header.total_size);
        ESP_LOGI(TAG, "Wav header %x", wav_file_header.wave);
        ESP_LOGI(TAG, "Subchunk fmt header %x", wav_file_header.SubChunkID);
        ESP_LOGI(TAG, "Audio format = %d. PCM=1", wav_file_header.AudioFormat);
        ESP_LOGI(TAG, "Num of channels = %d. ", wav_file_header.NumChannels);
        ESP_LOGI(TAG, "Samplerate = %d. ", wav_file_header.SampleRate);
        ESP_LOGI(TAG, "Bits per sample = %d. ", wav_file_header.BitsPerSample);
        if (wav_file_header.NumChannels == 1) {
          wav_file_header.NumChannels = I2S_CHANNEL_MONO;
        } else {
          wav_file_header.NumChannels = I2S_CHANNEL_STEREO;
        }
        // I2S clocks and sample format set
        i2s_set_clk(i2s_num, wav_file_header.SampleRate, wav_file_header.BitsPerSample,
                    wav_file_header.NumChannels);
        // Start I2S
        i2s_zero_dma_buffer(i2s_num);
        i2s_start(i2s_num);
        // remember the current file position for repeat sound
        fgetpos(file_to_play.file_desc, &data_pos);
        do {
          // file position to start of first data header
          fsetpos(file_to_play.file_desc, &data_pos);
          // start processing wav file data blocks
          // If we can't read next header =  end of the file
          while (((fread(&wav_datablock_header, 1, sizeof(wav_datablock_header),
                         file_to_play.file_desc)) == sizeof(wav_datablock_header)) &&
                 (file_to_play.repeat >= 0)) {
            ESP_LOGI(TAG, "Wav datablock size = %d. ", wav_datablock_header.SubChunkSize);
            // fread is now positioned at the start of the data.
            size_total_read = 0;
            while (size_total_read < wav_datablock_header.SubChunkSize) {
              size_read = fread(&play_buf, 1, sizeof(play_buf), file_to_play.file_desc);
              i2s_write(i2s_num, &play_buf, size_read, &i2s_bytes_written, 2000);
              size_total_read += size_read;
              // test if we received a stop signal in the queue
              if ((xQueuePeek(play_sound_queue, &peek_in_queue, 0)) == pdTRUE) {
                if (peek_in_queue.type == stop_play) {
                  ESP_LOGI(TAG, "Peek Queue found Sound stop message");
                  // stop repeat and reading for current data block
                  file_to_play.repeat = -1;
                  size_total_read = wav_datablock_header.SubChunkSize;
                }
              }
            }
          }
          // Processed all datablocks
        } while (file_to_play.repeat == 1);
      }
      // End of wav file processing

      i2s_stop(i2s_num);
#ifdef MAX98357_SD_PIN
      gpio_set_level(MAX98357_SD_PIN, 0);
#endif
      // Always close the files send in the queue
      fclose(file_to_play.file_desc);
    }
    if (file_to_play.type == stop_play) {
      ESP_LOGI(TAG, "Sound stop message received");
      xQueueReset(play_sound_queue);
    }
  }
}

// Try to open file. And send message to play on queue.
void play_wav(char *wavsound, int repeat) {
  // Add filesystem path and .wav to flename
  char filename[MAX_FILEPATH_LENGTH + 1] = FILESYSTEM1_BASE;
  strcat(filename, "/");
  strcat(filename, wavsound);
  strcat(filename, ".wav");
  // fill queue message
  file_to_play_t message;
  message.type = wav;
  message.repeat = repeat;
  message.file_desc = fopen(filename, "r");
  if (message.file_desc) {
    // We can open the file
    ESP_LOGI(TAG, "Opening sound file %s", filename);
    xQueueSendToBack(play_sound_queue, &message, 0);
  } else {
    ESP_LOGE(TAG, "Error opening sound file %s", filename);
  }
}

// stop playing wav file as soon as possible
void stop_sound(void) {
  // send stop message to front of queue
  ESP_LOGI(TAG, "Sound stop function called");
  file_to_play_t message;
  message.type = stop_play;
  xQueueSendToFront(play_sound_queue, &message, 0);
}
