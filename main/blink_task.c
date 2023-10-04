#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

#define LED_GPIO GPIO_NUM_5
#define GPIO_OUTPUT_PIN_SEL (1ULL << LED_GPIO)
#define PB_GPIO GPIO_NUM_23
#define GPIO_INPUT_PIN_SEL (1ULL << PB_GPIO)
#define ON_DELAY 1000 // Time parameter for led to be on

enum states {
  OFs,
  ONa,
  ONs,
  ONe,
  OFa,
} states1;

void Task_Toggle_Debounce(void *pvParameters);

void Task_Toggle_Debounce(void *pvParamters) {
  static uint8_t input = 0;            // Input for toggle
  static uint8_t input_prev = 0;       // Detect Rising or Falling
  static uint8_t s_led_state = 0;      // output for led level
  TickType_t buttonPressStartTime = 0; // Button Start time
  TickType_t buttonTime = 0;           // for counting pressed time
  enum states toggle_state = OFs;

  while (1) {

    input = gpio_get_level(PB_GPIO);

    /* Debouncing */
    if ((input == 1) && (input_prev == 0)) {
      vTaskDelay(5 / portTICK_PERIOD_MS);
      if (gpio_get_level(PB_GPIO) == 1) {
        input = 1;
        input_prev = 1;
      }
    } else if ((input == 1) && (input_prev == 1)) {
      input = 1;
    } else if ((input == 0) && (input_prev == 1)) {
      vTaskDelay(5 / portTICK_PERIOD_MS);
      if (gpio_get_level(PB_GPIO) == 0) {
        input = 0;
        input_prev = 0;
      }
    } else {
      input = 0;
      input_prev = 0;
    }

    /* Toggle the LED state */
    switch (toggle_state) {
    case OFs: {
      if (input == 1) {
        toggle_state = ONa;
        s_led_state = 1;
      } else {
        toggle_state = OFs;
        s_led_state = 0;
      }
      break;
    }
    case ONa: {
      if (input == 1) {
        toggle_state = ONa;
      } else {
        toggle_state = ONs;
      }
      break;
    }
    case ONs: {
      if (input == 1) {
        toggle_state = ONe;
        buttonPressStartTime = xTaskGetTickCount();
      } else {
        toggle_state = ONs;
      }
      break;
    }
    case ONe: {
      buttonTime = xTaskGetTickCount() - buttonPressStartTime;
      if ((input == 1) && (buttonTime >= pdMS_TO_TICKS(ON_DELAY))) {
        toggle_state = OFa;
        s_led_state = 0;
      } else if (input == 0) {
        toggle_state = ONs;

      } else {
        toggle_state = ONe;
      }
      break;
    }
    case OFa: {
      if (input == 1) {
        toggle_state = OFa;
      }
      if (input == 0) {
        toggle_state = OFs;
      }
      break;
    }
    }

    /*Set output value to LED*/
    gpio_set_level(LED_GPIO, s_led_state);

    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void app_main(void) {
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpio_config(&io_conf);

  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  io_conf.mode = GPIO_MODE_OUTPUT;
  gpio_config(&io_conf);

  // Create a task to toggle the LED
  xTaskCreate(Task_Toggle_Debounce, "LED Toggle Task",
              configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL);

  while (1) {
  }
}
