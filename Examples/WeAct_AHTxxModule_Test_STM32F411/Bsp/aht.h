#ifndef __AHT_H
#define __AHT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

#define AHT_ADDR (0x38 << 1)

#define AHT_REG_START (0xac)
#define AHT_STEP_IDLE (0)
#define AHT_STEP_INIT (1)
#define AHT_STEP_GET_RESULT (2)

    void aht_init(void);
    uint8_t aht_run_loop(uint32_t tick);
    int16_t aht_get_temperature(void);
    uint16_t aht_get_humidness(void);

#ifdef __cplusplus
}
#endif

#endif
