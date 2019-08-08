/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <time.h>
#include "sdcard.h"
#include "sgcam.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../core/kv/kv.h"
#include "../core/log/log.h"

#include "camera.h"
#include "./led.h"

#define CAMERA_PIXEL_FORMAT CAMERA_PF_JPEG
#define CAMERA_FRAME_SIZE CAMERA_FS_SVGA
/* #define TASK_STACK_SIZE 4096

StaticTask_t xTaskBuffer;
StackType_t xStack[ TASK_STACK_SIZE ];
*/

static camera_pixelformat_t s_pixel_format;
static camera_config_t camera_config = {
  .ledc_channel = LEDC_CHANNEL_0,
  .ledc_timer = LEDC_TIMER_0,
  .pin_d0 = CONFIG_D0,
  .pin_d1 = CONFIG_D1,
  .pin_d2 = CONFIG_D2,
  .pin_d3 = CONFIG_D3,
  .pin_d4 = CONFIG_D4,
  .pin_d5 = CONFIG_D5,
  .pin_d6 = CONFIG_D6,
  .pin_d7 = CONFIG_D7,
  .pin_xclk = CONFIG_XCLK,
  .pin_pclk = CONFIG_PCLK,
  .pin_vsync = CONFIG_VSYNC,
  .pin_href = CONFIG_HREF,
  .pin_sscb_sda = CONFIG_SDA,
  .pin_sscb_scl = CONFIG_SCL,
  .pin_reset = CONFIG_RESET,
  .xclk_freq_hz = CONFIG_XCLK_FREQ,
};

static void sgcam_task(void *param);

void take_picture() {
  led_open();
  esp_err_t err = camera_run();
  if (err != ESP_OK) {
    ESP_LOGD(SGO_LOG_EVENT, "@SGCAM Camera capture failed with error = %d", err);
    return;
  }
  led_close();
  mount_sdcard();
  char file_name[64] = {0};
  sprintf(file_name, "/sdcard/%ld.jpg", time(NULL));
  write_file(file_name, camera_get_fb(), camera_get_data_size());
  unmount_sdcard();
}

void init_sgcam() {
  ESP_LOGI(SGO_LOG_EVENT, "@SGCAM Initializing sgcam module\n");
  esp_err_t err;

  camera_model_t camera_model;
  err = camera_probe(&camera_config, &camera_model);
  if (err != ESP_OK) {
    ESP_LOGE(SGO_LOG_EVENT, "@SGCAM Camera probe failed with error 0x%x", err);
    return;
  }

  if (camera_model == CAMERA_OV2640) {
    ESP_LOGI(SGO_LOG_EVENT, "@SGCAM Detected OV2640 camera, using JPEG format");
    s_pixel_format = CAMERA_PIXEL_FORMAT;
    camera_config.frame_size = CAMERA_FRAME_SIZE;
    if (s_pixel_format == CAMERA_PF_JPEG)
      camera_config.jpeg_quality = 80;
  } else {
    ESP_LOGE(SGO_LOG_EVENT, "@SGCAM Camera not supported");
    return;
  }

  camera_config.pixel_format = s_pixel_format;

  err = camera_init(&camera_config);
  if (err != ESP_OK) {
    ESP_LOGE(SGO_LOG_EVENT, "@SGCAM Camera init failed with error 0x%x", err);
    return;
  }

  led_init();

  BaseType_t xReturned = xTaskCreate(sgcam_task, "SGCAM", 4096, NULL, 10, NULL);
  if( xReturned != pdPASS ) {
    ESP_LOGE(SGO_LOG_EVENT, "@SGCAM Failed to start SGCAM task");
  }
  /* TaskHandle_t handle = xTaskCreateStatic(sgcam_task, "SGCAM", TASK_STACK_SIZE, NULL, 10, xStack, &xTaskBuffer);
  if( handle == NULL ) {
    ESP_LOGE(SGO_LOG_EVENT, "@SGCAM Failed to start SGCAM task");
  } */
}

static void sgcam_task(void *param) {
  ESP_LOGI(SGO_LOG_EVENT, "@SGCAM Task start");

  while (true) {
    ESP_LOGI(SGO_LOG_EVENT, "@SGCAM Taking picture...");
    take_picture();
    ESP_LOGI(SGO_LOG_EVENT, "@SGCAM Picture taken.");
    vTaskDelay(600 * 1000 / portTICK_PERIOD_MS);
  }
}
