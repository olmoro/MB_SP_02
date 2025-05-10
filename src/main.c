#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs.h"

//  #include "mb.h"
//  #include "mb_controller.h"
//  #include "esp_modbus_slave.h"


#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "project_config.h"
#include "board.h"
#include "uart1_task.h"
#include "uart2_task.h"
//#include "nvs_settings.h"
#include "device.h"

static const char *TAG = "UART Gateway";
// static QueueHandle_t uart1_queue, uart2_queue;
// static SemaphoreHandle_t uart1_mutex, uart2_mutex;

// // Структура для хранения Modbus (и Sp?) регистров
// static uint16_t holding_regs[1] = {0};  // Holding регистры (16-bit)

void app_main(void)
{
    // Инициализация NVS
    //nvs_init_modbus_settings();

    // ESP_ERROR_CHECK(init_nvs());
    // ESP_LOGI(TAG, "NVS инициализирован");

    // // Загрузка параметра из NVS
    // int32_t stored_value = 0;
    // if (load_parameter_from_nvs(&stored_value) == ESP_OK) 
    // {
    //     holding_regs[PARAM_REG_ADDR] = (uint16_t)stored_value;
    //     ESP_LOGI(TAG, "Загружен параметр из NVS: %d", (int)stored_value);
    // } 
    // else 
    // {
    //     ESP_LOGW(TAG, "Параметр не найден, используется значение по умолчанию");
    // }


    // Инициализация периферии
    boardInit();
    uart_mb_init();
    uart_sp_init();

    // Создание задач
    xTaskCreate(uart1_task, "UART1 Task", 4096, NULL, 5, NULL);
    xTaskCreate(uart2_task, "UART2 Task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "System initialized");

    /* Проверка RGB светодиода */
    ledsBlue();
}
