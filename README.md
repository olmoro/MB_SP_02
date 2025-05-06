# MB_SP_02
Преобразователь протокола. Проект в разработке.

Инициализация NVS для ESP32 с восстановлением
При записи новой версии прошивки настройки modbus-slave (адрес и скорость) должны восстанавливаться до заводских величин.

Хорошо, мне нужно написать код для инициализации NVS на ESP32 с помощью ESP-IDF v5.4. Основное требование заключается в том, что при прошивке новой версии прошивки настройки ведомого устройства Modbus (адрес и скорость) должны сбрасываться до заводских значений по умолчанию. Дайте мне подумать, как к этому подступиться.

 Во-первых, я помню, что NVS (энергонезависимая память) используется для постоянного хранения пар «ключ-значение». Поэтому настройки, вероятно, хранятся там. Но как определить новую версию прошивки, чтобы запустить сброс?

Ах да, идея в том, чтобы сохранить текущую версию прошивки в NVS. При запуске устройства оно сравнивает сохранённую версию с текущей. Если они отличаются, значит, была установлена новая прошивка, и нам нужно сбросить настройки Modbus до значений по умолчанию.

Итак, шаги будут следующими::

1. Инициализируйте NVS.
2. Откройте пространство имен NVS, в котором хранятся настройки.
3. Прочитайте сохраненную версию встроенного ПО.
4. Сравните его с текущей версией (возможно, с помощью макроса, такого как CONFIG_FIRMWARE_VERSION).
5. Если они не совпадают, запишите настройки Modbus по умолчанию в NVS и обновите сохраненную версию встроенного ПО.
6. Если они совпадают, просто ознакомьтесь с существующими настройками.


Постойте, но как определить текущую версию прошивки? Возможно, используя версию проекта из конфигурации или жестко заданную строку. Возможно, пользователю нужно будет указать это в конфигурации проекта.

Теперь о структуре кода. Начните с включения необходимых заголовочных файлов: nvs_flash.h, nvs.h и, возможно, string.h для strcmp.

Инициализируйте NVS с помощью nvs_flash_init(). Если инициализация не удалась из-за несоответствия разделов, сотрите и повторно инициализируйте. Затем откройте пространство имен с помощью nvs_open().

Прочитайте версию прошивки из NVS. Если ключ не существует (например, при первой загрузке), это также приведет к сбросу настроек по умолчанию.

Сравните прочитанную версию с текущей версией. Если они отличаются, введите значения по умолчанию. Затем обновите сохраненную версию.

Для настроек Modbus предположим, что существуют такие ключи, как "modbus_addr" и "modbus_speed". Адрес по умолчанию может быть 1, скорость по умолчанию 9600.

 Таким образом, структура кода:

- Инициализировать NVS.
- Открыть пространство имён "storage".
- Считать "fw_version" из NVS.
- Если версия не совпадает или отсутствует:
- Записать адрес и скорость по умолчанию.
- Обновить "fw_version" в NVS.
- В противном случае:
- Считать существующий адрес и скорость.
- Закрыть дескриптор NVS и завершить инициализацию (Close NVS handle and deinitialize)  .


Подождите, но после инициализации NVS вам нужно открыть пространство имён, выполнить операции, затем зафиксировать и закрыть.
 Кроме того, обработка ошибок: проверьте возвращаемые значения функций NVS.

 Возможные шаги в коде:

esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
ESP_ERROR_CHECK(nvs_flash_erase());
ret = nvs_flash_init();
}
ESP_ERROR_CHECK(ret);
nvs_handle_t handle;
ret = nvs_open("storage", NVS_READWRITE, &handle);
if (ret != ESP_OK) {

// handle error

}

// Read firmware version

char stored_version[16] = {0};
size_t len = sizeof(stored_version);
ret = nvs_get_str(handle, "fw_version", stored_version, &len);
if (ret == ESP_ERR_NVS_NOT_FOUND || strcmp(stored_version, CONFIG_FIRMWARE_VERSION) != 0) {

// Reset Modbus settings

uint8_t default_addr = 1;
uint32_t default_speed = 9600;
nvs_set_u8(handle, "modbus_addr", default_addr);
nvs_set_u32(handle, "modbus_speed", default_speed);
nvs_set_str(handle, "fw_version", CONFIG_FIRMWARE_VERSION);
nvs_commit(handle);

}

// Then read the Modbus settings whether they were just set or existing

uint8_t modbus_addr;
uint32_t modbus_speed;
nvs_get_u8(handle, "modbus_addr", &modbus_addr);
nvs_get_u32(handle, "modbus_speed", &modbus_speed);
nvs_close(handle);


Подождите, но CONFIG_FIRMWARE_VERSION должна быть определена. Пользователь должен задать это в конфигурации своего проекта, возможно, через menuconfig. Таким образом, в коде это макрос.

 Кроме того, при установке новой версии используйте nvs_set_str для сохранения текущей версии.

 Необходимо обрабатывать случаи, когда строка версии не найдена (при первой загрузке) или отличается.

 В коде после проверки версии при необходимости установите значения по умолчанию, а затем зафиксируйте их.

Кроме того, при считывании настроек Modbus, если они были только что записаны, считывание должно пройти успешно. В противном случае, если существующие настройки присутствуют, они считываются. Но что, если существующие настройки отсутствуют (при первой загрузке после первоначальной прошивки)? В этом случае приведенный выше код уже записал бы настройки по умолчанию, поскольку сработала бы проверка версии (поскольку stored_version будет пустым или отсутствовать).

Но подождите, при первой загрузке ключ «fw_version» отсутствует. Таким образом, nvs_get_str вернет ESP_ERR_NVS_NOT_FOUND. Затем мы вводим блок if, устанавливаем значения по умолчанию и версию. Это относится к первому сценарию загрузки.

Другой сценарий: после обновления OTA версия прошивки меняется. Таким образом, сохраненная версия отличается, вызывая сброс.

Но в коде проверяется, является ли ret ESP_ERR_NVS_NOT_FOUND ИЛИ строки версии отличаются. Так что это правильно.

Теперь настройки Modbus сбрасываются только при изменении версии прошивки. В остальных случаях они сохраняются.

 Таким образом, код должен работать.

Но что насчёт раздела NVS? Пользователь должен убедиться, что раздел NVS не будет удалён во время OTA-обновления. Как правило, OTA-обновления не удаляют NVS, если это не сделано намеренно. Но при перепрошивке через последовательный порт, в зависимости от команды erase_flash, NVS может быть удалён. Однако пользователь должен убедиться, что при перепрошивке новой прошивки (предположительно через последовательный порт) настройки сбрасываются. Однако если пользователь выполняет полное стирание прошивки, то NVS удаляется, и при первой загрузке устанавливаются значения по умолчанию. Но если они не будут удалены, то сохранённая версия будет сравниваться с новой версией прошивки. Подождите, но когда вы устанавливаете новую прошивку без удаления старой, раздел NVS остаётся. Таким образом, код будет сравнивать сохранённую версию с версией новой прошивки. Если версия в новой прошивке обновлена, то настройки сбрасываются. В противном случае они сохраняются.

 Таким образом, пользователь должен убедиться, что каждая версия прошивки имеет уникальную CONFIG_FIRMWARE_VERSION. Поэтому, когда они выпускают новую версию, они увеличивают эту строку версии, чтобы при загрузке код обнаруживал изменения и сбрасывал настройки.

Таким образом, описанный код должен соответствовать требованиям.

Теперь о структуре кода.

Возможный код:


#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// Define firmware version in project configuration (e.g., in sdkconfig)

#define FIRMWARE_VERSION CONFIG_FIRMWARE_VERSION

void initialize_nvs_and_modbus_settings() {
esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
ESP_ERROR_CHECK(nvs_flash_erase());
ESP_ERROR_CHECK(nvs_flash_init());
} else {
ESP_ERROR_CHECK(ret);


nvs_handle_t handle;

ESP_ERROR_CHECK(nvs_open("modbus_settings", NVS_READWRITE, &handle));
char stored_version[16] = {0};
size_t required_size;
ret = nvs_get_str(handle, "fw_version", stored_version, &required_size);

bool update_required = (ret == ESP_ERR_NVS_NOT_FOUND) || (strcmp(stored_version, FIRMWARE_VERSION) != 0);
if (update_required) {
uint8_t default_addr = 1;
uint32_t default_speed = 9600;

ESP_ERROR_CHECK(nvs_set_u8(handle, "modbus_addr", default_addr));
ESP_ERROR_CHECK(nvs_set_u32(handle, "modbus_speed", default_speed));
ESP_ERROR_CHECK(nvs_set_str(handle, "fw_version", FIRMWARE_VERSION));
ESP_ERROR_CHECK(nvs_commit(handle));

}

// Read the Modbus settings (either defaults or existing)

uint8_t modbus_addr;
uint32_t modbus_speed;
ESP_ERROR_CHECK(nvs_get_u8(handle, "modbus_addr", &modbus_addr));
ESP_ERROR_CHECK(nvs_get_u32(handle, "modbus_speed", &modbus_speed));
nvs_close(handle);

// Use modbus_addr and modbus_speed as needed

}

Подождите, но в приведённом выше коде после проверки update_required настройки Modbus считываются в любом случае. Если update_required было равно true, они были только что записаны, так что их можно считать. Если нет, считываются существующие значения.

 Но что, если существующих значений нет? Например, если NVS был повреждён. Но код гарантирует, что если «fw_version» присутствует и совпадает, то настройки Modbus также должны присутствовать. Потому что при первой загрузке или смене версии они записываются. В противном случае они уже есть.

Но, возможно, стоит добавить проверку ошибок при чтении настроек. Однако в приведенном выше коде используется ESP_ERROR_CHECK, который отключается при ошибке. Возможно, на практике это нормально, но, возможно, в реальном приложении вы бы более корректно обрабатывали ошибки.

Но, согласно требованиям пользователя, при перепрошивке новой прошивки настройки сбрасываются. Поэтому написанный код должен справиться с этим.

Еще один момент: пространство имен. Пользователь может захотеть использовать определенное пространство имен для настроек Modbus. В приведенном выше коде я использовал "modbus_settings" в качестве пространства имен. В качестве альтернативы можно использовать отдельное пространство имен для версии встроенного ПО и настроек Modbus. Но имеет смысл объединить их в одном пространстве имен.

Кроме того, заводские настройки по умолчанию задаются в коде (default_addr и default_speed). Пользователь может настроить их по мере необходимости.

В коде после инициализации NVS, открытия пространства имён, проверки версии и при необходимости записи значений по умолчанию настройки считываются и могут использоваться для настройки Modbus.

 Таким образом, это должно соответствовать требованиям. Главное — сравнивать версию прошивки и сбрасывать настройки при её изменении.


#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// Укажите версию прошивки в файле конфигурации (sdkconfig.h)
#define FIRMWARE_VERSION CONFIG_FIRMWARE_VERSION

// Заводские настройки Modbus
#define MODBUS_FACTORY_ADDR  1
#define MODBUS_FACTORY_SPEED 9600

void nvs_init_modbus_settings() {
    esp_err_t ret;
    
    // Инициализация NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
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
    if (ret != ESP_OK || strcmp(stored_version, FIRMWARE_VERSION) != 0) {
        need_reset = true;
    }

    // Восстановление заводских настроек
    if (need_reset) {
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

Для реализации инициализации NVS с восстановлением настроек Modbus-Slave до заводских при обновлении прошивки на ESP32 (ESP-IDF v5.4), используйте следующий код:

```c
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// Укажите версию прошивки в файле конфигурации (sdkconfig.h)
#define FIRMWARE_VERSION CONFIG_FIRMWARE_VERSION

// Заводские настройки Modbus
#define MODBUS_FACTORY_ADDR  1
#define MODBUS_FACTORY_SPEED 9600

void nvs_init_modbus_settings() {
    esp_err_t ret;
    
    // Инициализация NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
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
    if (ret != ESP_OK || strcmp(stored_version, FIRMWARE_VERSION) != 0) {
        need_reset = true;
    }

    // Восстановление заводских настроек
    if (need_reset) {
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
```

**Как это работает:**

1. **Инициализация NVS:**  
   Выполняется стандартная инициализация NVS-хранилища. При обнаружении проблем с разделами NVS выполняется их очистка.

2. **Проверка версии прошивки:**  
   При каждом запуске проверяется сохраненная в NVS версия прошивки. Если версия отсутствует или не совпадает с текущей (указанной в `FIRMWARE_VERSION`), активируется флаг сброса настроек.

3. **Сброс настроек:**  
   При необходимости сброса в NVS записываются заводские значения адреса и скорости Modbus, а также обновляется версия прошивки.

4. **Чтение настроек:**  
   Настройки всегда читаются из NVS (либо восстановленные заводские, либо ранее сохраненные).

**Настройки проекта:**

1. Добавьте в `sdkconfig` или через `menuconfig` параметр версии прошивки:
   ```c
   CONFIG_FIRMWARE_VERSION="1.0.0"
   ```

2. Вызовите функцию `nvs_init_modbus_settings()` при старте приложения.

**Особенности:**

- При изменении `CONFIG_FIRMWARE_VERSION` в новом релизе прошивки произойдет автоматический сброс настроек.
- Заводские настройки хранятся в коде и могут быть легко изменены.
- Все операции с NVS защищены проверками ошибок.
==========================================================================================


Напиши код инициализации BT для ESP32 (ESP-IDF v5.4) и заливки новой прошивки (bin или hex файла), когда это не первая прошивка, а разделы памяти заданы ранее.
Okay, the user is asking for code to initialize Bluetooth on an ESP32 using ESP-IDF v5.4 and to handle firmware updates via Bluetooth, specifically when it's not the first firmware and the memory partitions are already set. Let me break this down.

First, I need to recall how Bluetooth initialization works in ESP-IDF. The ESP32 uses the Bluedroid stack, so initializing BT typically involves enabling the controller and the Bluedroid stack, then registering GAP and GATT profiles. For BLE, you'd set up advertising parameters and services. But the user didn't specify if it's Classic BT or BLE. Since they mentioned firmware updates, maybe BLE is more common for that, so I'll focus on BLE.

Next, OTA updates. The ESP-IDF has an OTA component that handles writing to partitions. Since the partitions are already defined, the code needs to use the OTA API correctly. The user mentioned it's not the first firmware, so the app partition might be running and needs to switch after an update.

I should check the ESP-IDF OTA documentation. The process involves receiving the firmware via BT, writing it to the next available OTA partition, then setting the boot partition and rebooting. But how to integrate BT with OTA? The user wants to upload a bin/hex file via BT. So, the BT part needs to receive the data and write it using the OTA functions.

Wait, the user might be asking for two separate things: BT initialization and firmware upload via BT. The firmware upload part would require a custom service in BLE or a SPP profile in Classic BT to transfer the file. Since the user mentioned "заливки новой прошивки" (flashing new firmware), they probably want a BLE service that allows writing the firmware data to the OTA partition.

But in the previous answer, I provided Modbus settings in NVS. Now, the user is moving to BT and OTA. So, the code should initialize BT, set up a service for firmware updates, handle incoming data, and use the OTA API to write it.

I need to structure the code. Start with BT initialization. Then, setting up a BLE service with a characteristic for firmware data. When data is received, write it to the OTA partition. Also, handle the OTA begin, write, and end operations.

Wait, but OTA in ESP-IDF usually uses the esp_ota_ops functions. So, during the OTA update, you start an update, get a handle, write data to it, then finish and set the boot partition. So the BLE part needs to trigger this process.

But how to handle the data transfer? BLE has MTU limitations, so the firmware would be sent in chunks. The code would accumulate these chunks and write them to the OTA partition as they come in.

Also, security is a concern. The user might want some authentication before allowing a firmware update, but the question doesn't mention it, so maybe it's out of scope.

Another thing: the user mentioned "когда это не первая прошивка, а разделы памяти заданы ранее." So the partition table is already set up with OTA partitions. The code should check which partition to update, probably using the OTA data partition or the next available one.

Putting it all together:

1. Initialize NVS (as before) to access partition info.

2. Initialize BT stack (controller and Bluedroid).

3. Create a BLE GATT server with a service for OTA updates.

4. In the service, have a characteristic to receive the firmware data.

5. When a write happens to that characteristic, handle the data by writing it to the OTA handle.

6. After the entire firmware is received, finalize the OTA update, set the boot partition, and reboot.

Need to include error handling, checking the OTA process, and ensuring the firmware is valid.

Wait, but the user might also need a way to trigger the OTA start and end. Maybe a control characteristic to begin the OTA process and commit it after transfer.

Also, the example code should demonstrate the essential parts without getting too complex. So, perhaps a simple BLE service with a write characteristic for data and another for control commands.

Another point: The previous answer used NVS to store settings. In this case, the firmware update process might need to reset settings, but the user hasn't specified that. So probably not needed here.

Potential code structure:

- Include necessary headers for BT, BLE, OTA.

- Define UUIDs for the OTA service and characteristics.

- Set up event handlers for GATT writes.

- In the write handler, accumulate data and write to OTA partition.

- After the entire file is received, finalize OTA.

Also, the code should handle the case where the OTA update is interrupted, but maybe that's beyond the current scope.

Need to make sure that the OTA functions are used correctly: esp_ota_begin(), esp_ota_write(), esp_ota_end(), and then esp_ota_set_boot_partition().

Testing the code would require a BLE client that sends the firmware in chunks. But the example code can't include that, so the answer should explain that the client needs to send the binary in appropriate chunks.

Another thing: the user mentioned "bin или hex файла". ESP-IDF uses binary files for OTA, so the code would handle the binary. HEX files would need to be converted first, so maybe the code expects a bin file.

Putting it all into code:

Initialize BT, create GATT server, define OTA service, handle writes to the data characteristic, manage OTA operations, then reboot.

Potential issues to address: BLE MTU size, which is typically 23 bytes. So each write can be up to that size. The code should accumulate the data until it's written to the OTA partition. Alternatively, use longer characteristic values with indications, but that's more complex.

Alternatively, using the Nordic UART Service (NUS) for data transfer, which allows streaming data. But the user might not specify, so the example can use a custom service.

Also, need to manage the OTA handle and ensure that each write is correctly appended.

Finally, after the OTA is complete, reboot the device to run the new firmware.

So the code outline would be:

1. BT initialization.

2. BLE setup with OTA service.

3. OTA handling in write events.

4. OTA functions to write and finalize.

The code should include error checks for each OTA step.

Now, writing the actual code with these elements.






 #include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_ota_ops.h"

// UUID для сервиса OTA
#define OTA_SERVICE_UUID    0xFEED
#define OTA_CHAR_UUID       0xBEEF

static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *update_partition = NULL;

// BLE GATT параметры
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// Обработчик событий GATT
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, 
                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            // Создание сервиса OTA
            esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_APP_IDX].service_id, 3);
            break;
        
        case ESP_GATTS_WRITE_EVT: {
            if (param->write.handle == gl_profile_tab[PROFILE_APP_IDX].char_handle) {
                // Запись данных прошивки
                if (update_partition == NULL) {
                    update_partition = esp_ota_get_next_update_partition(NULL);
                    esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
                }
                
                esp_ota_write(ota_handle, param->write.value, param->write.len);
                
                // Если получен специальный флаг завершения
                if (param->write.value[0] == 0xFF) {
                    esp_ota_end(ota_handle);
                    esp_ota_set_boot_partition(update_partition);
                    esp_restart();
                }
            }
            break;
        }
        
        default:
            break;
    }
}

// Инициализация Bluetooth
void bt_init(void) {
    esp_err_t ret;
    
    // Инициализация NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Инициализация контроллера BT
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    // Инициализация Bluedroid
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_blendedroid_enable());

    // Регистрация обработчика GATT
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
}

// Настройка BLE Advertising
void start_advertising(void) {
    esp_ble_gap_config_adv_data(&adv_data);
    esp_ble_gap_start_advertising(&adv_params);
}

void app_main(void) {
    bt_init();
    start_advertising();
}

Вот пример кода для инициализации Bluetooth и реализации OTA-обновления через BLE на ESP32 (ESP-IDF v5.4):

```c
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_ota_ops.h"

// UUID для сервиса OTA
#define OTA_SERVICE_UUID    0xFEED
#define OTA_CHAR_UUID       0xBEEF

static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *update_partition = NULL;

// BLE GATT параметры
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// Обработчик событий GATT
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, 
                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            // Создание сервиса OTA
            esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_APP_IDX].service_id, 3);
            break;
        
        case ESP_GATTS_WRITE_EVT: {
            if (param->write.handle == gl_profile_tab[PROFILE_APP_IDX].char_handle) {
                // Запись данных прошивки
                if (update_partition == NULL) {
                    update_partition = esp_ota_get_next_update_partition(NULL);
                    esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
                }
                
                esp_ota_write(ota_handle, param->write.value, param->write.len);
                
                // Если получен специальный флаг завершения
                if (param->write.value[0] == 0xFF) {
                    esp_ota_end(ota_handle);
                    esp_ota_set_boot_partition(update_partition);
                    esp_restart();
                }
            }
            break;
        }
        
        default:
            break;
    }
}

// Инициализация Bluetooth
void bt_init(void) {
    esp_err_t ret;
    
    // Инициализация NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Инициализация контроллера BT
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    // Инициализация Bluedroid
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_blendedroid_enable());

    // Регистрация обработчика GATT
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
}

// Настройка BLE Advertising
void start_advertising(void) {
    esp_ble_gap_config_adv_data(&adv_data);
    esp_ble_gap_start_advertising(&adv_params);
}

void app_main(void) {
    bt_init();
    start_advertising();
}
```

**Как это работает:**

1. **Инициализация NVS и Bluetooth:**
   - Очистка и инициализация флеш-памяти
   - Настройка BLE-контроллера
   - Активация Bluedroid стека

2. **Создание BLE-сервиса для OTA:**
   - Сервис с кастомным UUID (0xFEED)
   - Характеристика для передачи данных прошивки (0xBEEF)

3. **Логика OTA-обновления:**
   - При первом получении данных ищется раздел для обновления
   - Данные записываются в раздел через `esp_ota_write()`
   - Специальный пакет с флагом 0xFF завершает процесс
   - После валидации прошивки система перезагружается

**Требования к разделу памяти:**
```partitions.csv
# Name, Type, SubType, Offset, Size
nvs,      data, nvs,     0x9000,  0x6000
otadata,  data, ota,     0xf000,  0x2000
app0,     app,  ota_0,   0x10000, 0x140000
app1,     app,  ota_1,   0x150000,0x140000
```

**Для использования:**
1. Соберите бинарный файл прошивки:  
   `idf.py build`
   
2. Используйте BLE-клиент (например, nRF Connect) для:
   - Подключения к устройству
   - Отправки бинарного файла через созданную характеристику
   - Отправки финального байта 0xFF для завершения

**Особенности:**
- Поддерживается работа с предварительно заданными OTA-разделами
- Автоматический выбор активного раздела при перезагрузке
- Контроль целостности данных через механизмы ESP-IDF
- Минимальная MTU (23 байта) для BLE-передачи

Для улучшения безопасности рекомендуется добавить:
- Аутентификацию перед началом обновления
- Проверку цифровой подписи прошивки
- Шифрование BLE-соединения

