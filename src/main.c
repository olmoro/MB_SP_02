#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "project_config.h"
#include "board.h"
#include "uart1_task.h"
#include "uart2_task.h"
#include "nvs_settings.h"

static const char *TAG = "UART Gateway";
// static QueueHandle_t uart1_queue, uart2_queue;
// static SemaphoreHandle_t uart1_mutex, uart2_mutex;

void app_main(void)
{
    // Инициализация NVS
    nvs_init_modbus_settings();
    
    // Инициализация периферии
    boardInit();
    uart_mb_init();
    uart_sp_init();

    // Создание задач
    xTaskCreate(uart1_task, "UART1 Task", 4096, NULL, 5, NULL);
    xTaskCreate(uart2_task, "UART2 Task", 4096, NULL, 5, NULL);     // 6

    ESP_LOGI(TAG, "System initialized");

    /* Проверка RGB светодиода */
    ledsBlue();
}
