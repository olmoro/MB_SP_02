#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t init_nvs(void);
    esp_err_t load_parameter_from_nvs(int32_t *value); 


#ifdef __cplusplus
}
#endif

#endif // !_DEVICE_H_
