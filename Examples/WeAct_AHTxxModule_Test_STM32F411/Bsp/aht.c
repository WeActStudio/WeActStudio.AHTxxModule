/*---------------------------------------
- WeAct Studio Official Link
- taobao: weactstudio.taobao.com
- aliexpress: weactstudio.aliexpress.com
- github: github.com/WeActStudio
- gitee: gitee.com/WeAct-TC
- blog: www.weact-tc.cn
---------------------------------------*/

#include "aht.h"

#include "i2c.h"

static uint8_t aht_run_step;
static uint32_t aht_tick;

static int16_t temperature;
static uint16_t humidness;

static void I2C_Error_Check(HAL_StatusTypeDef status)
{
  if (status == HAL_BUSY)
  {
    // I2C BUSY
    HAL_I2C_DeInit(&hi2c1);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    for (uint8_t i = 0; i < 8; i++)
    {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
      HAL_Delay(0);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    }
    HAL_I2C_Init(&hi2c1);
  }
  else if (status == HAL_ERROR)
  {
  }
}

static uint8_t i2c_write(uint8_t device_addr, uint8_t register_addr, uint8_t *pDat, uint8_t len)
{
  HAL_StatusTypeDef result;
  result = HAL_I2C_Mem_Write(&hi2c1, device_addr, register_addr, 1, pDat, len, 10);
  I2C_Error_Check(result);
  if (result == HAL_OK)
    return 0;
  else
    return 1;
}

static uint8_t i2c_read(uint8_t device_addr, uint8_t register_addr, uint8_t *pDat, uint8_t len)
{
  HAL_StatusTypeDef result;
  if (register_addr)
  {
    result = HAL_I2C_Mem_Read(&hi2c1, device_addr, register_addr, 1, pDat, len, 10);
  }
  else
  {
    result = HAL_I2C_Master_Receive(&hi2c1, device_addr, pDat, len, 10);
  }
  I2C_Error_Check(result);
  if (result == HAL_OK)
    return 0;
  else
    return 1;
}

static uint8_t aht_start_measure(void)
{
  uint8_t reg_data[2];
  reg_data[0] = 0x33;
  reg_data[1] = 0x00;
  return i2c_write(AHT_ADDR, AHT_REG_START, reg_data, 2);
}

static uint8_t CheckCrc8(uint8_t *pDat, uint8_t Lenth)
{
  uint8_t crc = 0xff, i, j;

  for (i = 0; i < Lenth; i++)
  {
    crc = crc ^ *pDat;
    for (j = 0; j < 8; j++)
    {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x31;
      else
        crc <<= 1;
    }
    pDat++;
  }
  return crc;
}

static uint8_t aht_get_result(void)
{
  uint8_t reg_data[7];
  uint8_t crc_result;
  uint64_t reg_temperature_data;
  uint64_t reg_humidness_data;
  if (i2c_read(AHT_ADDR, 0x00, reg_data, 7) == 0)
  {
    if ((reg_data[0] & 0x98) == 0x18)
    {
      crc_result = CheckCrc8(reg_data, 6);
      if (crc_result == reg_data[6])
      {
        reg_humidness_data = reg_data[1];
        reg_humidness_data = (reg_humidness_data << 8) + reg_data[2];
        reg_humidness_data = (reg_humidness_data << 4) + (reg_data[3] >> 4);
        reg_humidness_data = ((reg_humidness_data + 1) * 10000) >> 20;
        humidness = reg_humidness_data & 0xffff;

        reg_temperature_data = reg_data[3] & 0x0f;
        reg_temperature_data = (reg_temperature_data << 8) + reg_data[4];
        reg_temperature_data = (reg_temperature_data << 8) + reg_data[5];
        reg_temperature_data = ((reg_temperature_data + 1) * 20000) >> 20;
        temperature = reg_temperature_data & 0xffff;
        temperature -= 5000;

        return 0;
      }
      else
        return 3;
    }
    else
      return 2;
  }
  return 1;
}

void aht_init(void)
{
  aht_run_step = AHT_STEP_INIT;
  aht_tick = 10;
}

uint8_t aht_run_loop(uint32_t tick)
{
  if (tick >= aht_tick)
  {
    aht_tick = tick + 10;

    switch (aht_run_step)
    {
    case AHT_STEP_INIT:
    {
      aht_tick += 100;
      aht_run_step = AHT_STEP_IDLE;
    }
    break;
    case AHT_STEP_IDLE:
    {
      aht_start_measure();
      aht_run_step = AHT_STEP_GET_RESULT;
    }
    break;
    case AHT_STEP_GET_RESULT:
    {
      if (aht_get_result())
      {
        aht_run_step = AHT_STEP_GET_RESULT;
      }
      else
      {
        aht_run_step = AHT_STEP_IDLE;
      }
    }
    break;
    default:
      break;
    }
  }
  return aht_run_step;
}

// temperature 0.00 Degree Celsius
int16_t aht_get_temperature(void)
{
  return temperature;
}

// humidness 0.00%
uint16_t aht_get_humidness(void)
{
  return humidness;
}
