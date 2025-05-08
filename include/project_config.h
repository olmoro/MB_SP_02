/*
   ---------------------------------------------------------------------------------
                            Файл конфигурации проекта
   ---------------------------------------------------------------------------------
*/

#pragma once

#include <stdint.h>
#include "esp_task.h"

#include <time.h>
#include <stdio.h>
// #include <stdint.h>
#include <stdbool.h>
#include "esp_bit_defs.h"
// ---------------------------------------------------------------------------------
//                                  Версии
// ---------------------------------------------------------------------------------
#define FIRMWARE_VERSION "MB_SP_02.04"
// 202500506.06:  add nvs init   без подключения            RAM:  3.5%  Flash: 14.5%
// 202500422.06:  Без printf()                              RAM:  3.5%  Flash: 13.1%


// modbus -> 01 10 00 02 00 0A 14 01 00 86 1F 1D 33 33 32 02 09 30 30 30 09 30 30 33 0C 03 00 59 2D
// sp     -> 10 01 00 86 10 1F 1D 33 33 32 10 02 09 30 30 30 09 30 30 33 0C 10 03 42 16
// reply  <- FF FF 10 01 86 00 10 1F 03 33 33 32 10 02 09 30 09 30 30 33 0C 09 32 30 36 30 31 30 30 30 30 35 09 20 0C 10 03 32 61


//#define TEST_MODBUS

// ---------------------------------------------------------------------------------
//                                  GPIO
// ---------------------------------------------------------------------------------
// Плата SPN.55
// Светодиоды
#define RGB_RED_GPIO 4   // Красный, катод на GND (7mA)
#define RGB_GREEN_GPIO 2 // Зелёный, катод на GND (5mA)
#define RGB_BLUE_GPIO 27 // Синий,   катод на GND (4mA)

// Входы
// #define CONFIG_GPIO_IR 19 // Вход ИК датчика (резерв)

// UART1
#define CONFIG_MB_UART_RXD 25
#define CONFIG_MB_UART_TXD 26
#define CONFIG_MB_UART_RTS 33

// UART2
#define CONFIG_SP_UART_RXD 21
#define CONFIG_SP_UART_TXD 23
#define CONFIG_SP_UART_RTS 22

#define CONFIG_UART_DTS 32 // Не используется

#define A_FLAG_GPIO        17
#define B_FLAG_GPIO        16


// ---------------------------------------------------------------------------------
//                             Заводские настройки
// ---------------------------------------------------------------------------------

// Заводские настройки Modbus
#define MODBUS_FACTORY_ADDR  1
#define MODBUS_FACTORY_SPEED 9600

// Заводские настройки SP
#define SP_FACTORY_ADDR  0
#define SP_FACTORY_SPEED 115200
// ---------------------------------------------------------------------------------
//                                    Общие
// ---------------------------------------------------------------------------------
#define BUF_SIZE (240) // размер буфера
#define BUF_MIN_SIZE (4) // минимальный размер буфера
#define MAX_PDU_LENGTH 240

//#define SLAVE_ADDRESS_2
// ---------------------------------------------------------------------------------
//                                    MODBUS
// ---------------------------------------------------------------------------------

// #define PRB // Тестовый. Их принятого фрейма исключается вся служебная информация

#define MB_PORT_NUM UART_NUM_1
#define MB_BAUD_RATE 9600

// #ifdef SLAVE_ADDRESS_2
//    #define SLAVE_ADDRESS 0x01 + 1
// #else
//    #define SLAVE_ADDRESS 0x01
// #endif

#define MB_QUEUE_SIZE 2
#define MB_FRAME_TIMEOUT_MS 4 // 3.5 символа при 19200 бод


#define MB_MAX_LEN       250
#define SP_MAX_LEN       MB_MAX_LEN * 2
#define UART_BUF_SIZE    (1024 * 2)
#define QUEUE_SIZE       10
#define RESP_TIMEOUT_MS  100


// ---------------------------------------------------------------------------------
//                                    SP
// ---------------------------------------------------------------------------------

#define SP_PORT_NUM UART_NUM_2
#define SP_BAUD_RATE 115200

#define SP_ADDRESS 0x00
#define SP_QUEUE_SIZE 2
#define SP_FRAME_TIMEOUT_MS 10   // По факту

// Константы протокола
#define SOH 0x01        // Байт начала заголовка
#define ISI 0x1F        // Байт указателя кода функции
#define STX 0x02        // Байт начала тела сообщения
#define ETX 0x03        // Байт конца тела сообщения
#define DLE 0x10        // Байт символа-префикса
#define CRC_INIT 0x1021 // Битовая маска полинома
// #define FRAME 128       //

