
#ifndef __RADIOENGEMODEM_H
#define __RADIOENGEMODEM_H

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

typedef enum
{
  RADIO_RESET=0x1,
  RADIO_CONFIGURING=0x2,
  RADIO_JOINING=0x4,
  RADIO_JOINED=0x8  
} RADIO_STATE;

#define RADIO_STATE_ALL (RADIO_RESET | RADIO_CONFIGURING | RADIO_JOINING | RADIO_JOINED)

typedef enum
{
  JOINED_TX,
  JOINED_WAIT_START,
  JOINED_WAITING,
  JOINED_RX  
} JOINED_STATE;

osStatus_t LoRaSend(uint32_t LoraWANPort,uint8_t* msg);
osStatus_t LoRaSendB(uint32_t LoraWANPort, uint8_t* msg, size_t size);
void LoRaWaitDutyCycle();
osStatus_t LoRaSendNow(uint32_t LoraWANPort, uint8_t* msg);
osStatus_t LoRaSendBNow(uint32_t LoraWANPort, uint8_t* msg, size_t size);

#endif