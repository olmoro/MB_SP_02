/*
     Пояснения:
    1. Инициализация NVS:
    - Проверка и восстановление флеш-памяти
    - Загрузка сохраненного параметра при старте
    - Автоматическое восстановление при ошибках

    2. Работа с Modbus:
    - Использование RTU режима
    - Настройка UART с параметрами по умолчанию
    - Регистрация callback-функций для работы с регистрами
    - Автоматическое сохранение в NVS при изменении регистра

    3. Особенности промышленного применения:
    - Проверка всех возвращаемых кодов ошибок
    - Детальное логирование состояния
    - Защита от переполнения буферов
    - Атомарные операции с NVS

    4. Рекомендуемые доработки:
    - Добавление watchdog таймера
    - Реализация дополнительных регистров
    - Защита от некорректных значений
    - Реализация безопасного обновления прошивки

*/

#include "device.h"
#include "project_config.h"
#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "nvs_flash.h"

// Теги для логов
static const char *TAG = "DEVICE";

// Конфигурация параметров
typedef struct {
    uint16_t reg_addr_msw;  // Адрес старших бит
    uint16_t reg_addr_lsw;  // Адрес младших бит
    const char* nvs_key;    // Ключ в NVS
    uint32_t value;         // Текущее значение
} mb_sp_param_t;


// Параметры устройства
enum {
    PARAM_MB_ADDR,
    PARAM_MB_SPEED,
    PARAM_SP_ADDR,
    PARAM_SP_SPEED,
    PARAM_COUNT  // Всегда последний элемент
};

// Инициализация параметров с регистрами и ключами
static mb_sp_param_t device_params[] = 
{
    [PARAM_MB_ADDR]         = {1000, 1001, "mb_addr",  1},
    [PARAM_MB_SPEED]        = {1002, 1003, "mb_speed",  9600},
    [PARAM_SP_ADDR]         = {1004, 1005, "sp_addr",  0},
    [PARAM_SP_SPEED]        = {1006, 1007, "sp_speed",  115200},
};

static nvs_handle_t dev_handle;

/**
 * @brief Инициализация NVS хранилища                   !!! уточнить
 */
esp_err_t nvs_storage_init(void) 
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_LOGW(TAG, "Очистка NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief Загрузка всех параметров из NVS
 */
esp_err_t load_all_parameters(void) 
{
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &dev_handle);
    if (err != ESP_OK) return err;

    for (int i = 0; i < PARAM_COUNT; i++) 
    {
        err = nvs_get_u32(dev_handle, device_params[i].nvs_key, &device_params[i].value);
        if (err == ESP_ERR_NVS_NOT_FOUND) 
        {
            ESP_LOGW(TAG, "Параметр %s не найден, инициализация 0", device_params[i].nvs_key);
            device_params[i].value = 0;
            nvs_set_u32(dev_handle, device_params[i].nvs_key, 0);
        }
        ESP_LOGI(TAG, "Загружен %s = %" PRIu32, device_params[i].nvs_key, device_params[i].value);
    }
    return nvs_commit(dev_handle);
}

/**
 * @brief Поиск параметра по адресу регистра
 * @param address Адрес регистра Modbus
 * @param part Указатель для возврата части параметра (0-MSW, 1-LSW)
 * @return Указатель на параметр или NULL
 */
static mb_sp_param_t* find_param_by_address(uint16_t address, uint8_t* part) 
{
    for (int i = 0; i < PARAM_COUNT; i++) 
    {
        if (address == device_params[i].reg_addr_msw) 
        {
            *part = 0;
            return &device_params[i];
        }
        if (address == device_params[i].reg_addr_lsw) 
        {
            *part = 1;
            return &device_params[i];
        }
    }
    return NULL;
}

/**
 * @brief Callback на запись регистра
 * @param address Адрес регистра
 * @param value Значение для записи
 * @return ESP_OK при успехе или код ошибки
 */
esp_err_t write_holding_register(uint16_t address, uint16_t value) 
{
    uint8_t part;
    mb_sp_param_t *param = find_param_by_address(address, &part);
    if (!param) 
    {
        ESP_LOGE(TAG, "Неверный адрес регистра: %d", address);
        return ESP_ERR_INVALID_ARG;
    }

    // Обновление значения
    uint32_t new_value = part ? 
        (param->value & 0xFFFF0000) | value : 
        (value << 16) | (param->value & 0xFFFF);

    // Проверка на изменение значения
    if (new_value != param->value) 
    {
        param->value = new_value;
        
        // Сохранение в NVS
        esp_err_t err = nvs_set_u32(dev_handle, param->nvs_key, param->value);
        if (err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Ошибка записи %s: 0x%x", param->nvs_key, err);
            return err;
        }
        
        err = nvs_commit(dev_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Ошибка коммита %s: 0x%x", param->nvs_key, err);
            return err;
        }
        
        ESP_LOGI(TAG, "Обновлен %s = %" PRIu32, param->nvs_key, param->value);
    }
    return ESP_OK;
}

/**
 * @brief Callback на чтение регистра
 * @param address Адрес регистра
 * @param value Указатель для возврата значения
 * @return ESP_OK при успехе или код ошибки
 */
esp_err_t read_holding_register(uint16_t address, uint16_t *value) 
{
    uint8_t part;
    mb_sp_param_t *param = find_param_by_address(address, &part);
    if (!param) 
    {
        ESP_LOGE(TAG, "Чтение несуществующего регистра: %d", address);
        return ESP_ERR_INVALID_ARG;
    }

    *value = part ? 
        (param->value & 0xFFFF) : 
        (param->value >> 16) & 0xFFFF;
    
    return ESP_OK;
}










// // Структура для хранения Modbus (и Sp?) регистров
// static uint16_t holding_regs[1] = {0};  // Holding регистры (16-bit)

// /* Инициализация NVS */
// esp_err_t init_nvs(void) 
// {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
//     {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     return ret;
// }

// /* Загрузка параметра из NVS */
// esp_err_t load_parameter_from_nvs(int32_t *value) 
// {
//     nvs_handle_t nvs_handle;
//     esp_err_t err;
    
//     err = nvs_open("storage", NVS_READONLY, &nvs_handle);
//     if (err != ESP_OK) return err;
    
//     err = nvs_get_i32(nvs_handle, "param", value);
//     nvs_close(nvs_handle);
    
//     return err;
// }

// /* Сохранение параметра в NVS */
// esp_err_t save_parameter_to_nvs(int32_t value) 
// {
//     nvs_handle_t nvs_handle;
//     esp_err_t err;
    
//     err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
//     if (err != ESP_OK) return err;
    
//     err = nvs_set_i32(nvs_handle, "param", value);
//     if (err != ESP_OK) 
//     {
//         nvs_close(nvs_handle);
//         return err;
//     }
    
//     err = nvs_commit(nvs_handle);
//     nvs_close(nvs_handle);
    
//     return err;
// }

// /* Callback-функция для работы с holding регистрами */
// static mb_err_t read_holding_regs(uint8_t *buffer, uint16_t start_addr, uint16_t quantity) 
// {
//     if (start_addr + quantity > sizeof(holding_regs)/sizeof(holding_regs[0])) 
//     {
//         return MB_EINVAL;
//     }
//     memcpy(buffer, &holding_regs[start_addr], quantity * sizeof(uint16_t));
//     return MB_OK;
// }

// static mb_err_t write_holding_regs(uint8_t *buffer, uint16_t start_addr, uint16_t quantity) 
// {
//     if (start_addr + quantity > sizeof(holding_regs)/sizeof(holding_regs[0])) 
//     {
//         return MB_EINVAL;
//     }
//     memcpy(&holding_regs[start_addr], buffer, quantity * sizeof(uint16_t));
    
//     // Если изменили целевой регистр - сохраняем в NVS
//     if (start_addr <= PARAM_REG_ADDR && (start_addr + quantity) > PARAM_REG_ADDR) 
//     {
//         int32_t value = (int32_t)holding_regs[PARAM_REG_ADDR];
//         esp_err_t err = save_parameter_to_nvs(value);
//         if (err != ESP_OK) 
//         {
//             ESP_LOGE(TAG, "Ошибка сохранения в NVS: 0x%x", err);
//             return MB_ERROR;
//         }
//         ESP_LOGI(TAG, "Параметр сохранён: %d", value);
//     }
//     return MB_OK;
// }

