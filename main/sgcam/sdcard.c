/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "../core/log/log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

void mount_sdcard(void)
{
  ESP_LOGI(SGO_LOG_EVENT, "Initializing SD card");

  ESP_LOGI(SGO_LOG_EVENT, "Using SPI peripheral");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();

  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = PIN_NUM_MISO;
  slot_config.gpio_mosi = PIN_NUM_MOSI;
  slot_config.gpio_sck  = PIN_NUM_CLK;
  slot_config.gpio_cs   = PIN_NUM_CS;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  sdmmc_card_t* card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(SGO_LOG_EVENT, "Failed to mount filesystem. "
          "If you want the card to be formatted, set format_if_mount_failed = true.");
    } else {
      ESP_LOGE(SGO_LOG_EVENT, "Failed to initialize the card (%s). "
          "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
    }
    return;
  }
}

void unmount_sdcard(void) {
  ESP_LOGI(SGO_LOG_EVENT, "Unmounting sdcard");
  // All done, unmount partition and disable SDMMC or SPI peripheral
  esp_vfs_fat_sdmmc_unmount();
}

void write_file(const char *name, const uint8_t *buf, size_t size) {
  // Use POSIX and C standard library functions to work with files.
  // First create a file.
  ESP_LOGI(SGO_LOG_EVENT, "Opening file %s", name);
  FILE *f = fopen(name, "wb");
  if (f == NULL) {
    ESP_LOGE(SGO_LOG_EVENT, "Failed to open file for writing");
    return;
  }
  size_t n = fwrite(buf, 1, size, f);
  ESP_LOGI(SGO_LOG_EVENT, "File written %d (was %d)", n, size);
  fclose(f);
}
