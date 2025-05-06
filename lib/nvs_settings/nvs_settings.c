/*
    Как это работает:
1. Инициализация NVS:  
   Выполняется стандартная инициализация NVS-хранилища. При обнаружении проблем с разделами 
   NVS выполняется их очистка.

2. Проверка версии прошивки:  
   При каждом запуске проверяется сохраненная в NVS версия прошивки. Если версия отсутствует 
   или не совпадает с текущей (указанной в `FIRMWARE_VERSION`), активируется флаг сброса настроек.

3. Сброс настроек:  
   При необходимости сброса в NVS записываются заводские значения адреса и скорости Modbus, 
   а также обновляется версия прошивки.

4. Чтение настроек:  
   Настройки всегда читаются из NVS (либо восстановленные заводские, либо ранее сохраненные).
*/

#include "nvs_settings.h"
#include "project_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

//#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
                        // Укажите версию прошивки в файле конфигурации (sdkconfig.h)
                        // #define FIRMWARE_VERSION CONFIG_FIRMWARE_VERSION
                        #define FIRMWARE_VERSION "100"

                        // Заводские настройки Modbus
                        #define MODBUS_FACTORY_ADDR  1
                        #define MODBUS_FACTORY_SPEED 9600



void nvs_init_modbus_settings()
{
    esp_err_t ret;
    
    // Инициализация NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } 
    else 
    {
        ESP_ERROR_CHECK(ret);
    }

    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("modbus_cfg", NVS_READWRITE, &nvs_handle));

    // Проверка версии прошивки
    char stored_version[16] = {0};
    size_t required_size;
    bool need_reset = false;

    ret = nvs_get_str(nvs_handle, "fw_ver", stored_version, &required_size);
    
    // Сброс настроек если:
    // 1. Версия не найдена (первый запуск)
    // 2. Версия не совпадает с текущей
    if (ret != ESP_OK || strcmp(stored_version, FIRMWARE_VERSION) != 0) 
    {
        need_reset = true;
    }

    // Восстановление заводских настроек
    if (need_reset) 
    {
        ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, "addr", MODBUS_FACTORY_ADDR));
        ESP_ERROR_CHECK(nvs_set_u32(nvs_handle, "speed", MODBUS_FACTORY_SPEED));
        ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "fw_ver", FIRMWARE_VERSION));
        ESP_ERROR_CHECK(nvs_commit(nvs_handle));
        ESP_LOGI("NVS", "Modbus settings reset to factory");
    }

    // Чтение настроек из NVS
    uint8_t modbus_addr;
    uint32_t modbus_speed;
    ESP_ERROR_CHECK(nvs_get_u8(nvs_handle, "addr", &modbus_addr));
    ESP_ERROR_CHECK(nvs_get_u32(nvs_handle, "speed", &modbus_speed));

    nvs_close(nvs_handle);

    ESP_LOGI("NVS", "Modbus address: %d, speed: %lu", modbus_addr, modbus_speed);
}
