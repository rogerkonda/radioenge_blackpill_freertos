#ifndef __UARTAT_H
#define __UARTAT_H

#include "cmsis_os.h"
#include "uartRingBufDMA.h"

typedef enum
{
  AT_IDLE,
  AT_WAITING_RESPONSE
} AT_FSM;

typedef enum
{
  ATZ,
  AT_CFM,
  AT_APPKEY,
  AT_APPEUI,
  AT_ADR,
  AT_NJM,
  AT_JOIN,
  AT_CHMASK,
  AT_SENDB,
  AT_SEND,
  AT,
  AT_COMMAND_UNDEFINED,
  NUM_AT_COMMANDS
} ATCommand;

typedef enum
{
  AT_OK,
  AT_TX_OK,
  AT_RX_OK,
  AT_ERROR,
  AT_JOINED,
  AT_TIMEOUT,
  AT_BUSY,
  AT_JOIN_ERROR,
  AT_RESET,
  AT_NO_NETWORK_JOINED,
  AT_RESPONSE_UNDEFINED,
  NUM_AT_RESPONSES
} ATResponse;



#define ATCMD_MEMPOOL_OBJECTS (8)
#define LORAPAYLOAD_MEMPOOL_OBJECTS (2)         // number of Memory Pool Objects 

typedef struct {                                // object data type
  uint32_t rcvPort;
  uint32_t number;
  uint32_t rcvDataLen;
  int32_t rcvRSSI;
  int32_t rcvSNR;
  uint8_t Buf[64];
} LORA_PAYLOAD_MEM_BLOCK_t; 

typedef struct CMD
{
  ATCommand command;
  ATResponse response;
  union {
    LORA_PAYLOAD_MEM_BLOCK_t* rx_payload;  
    UART_MEM_BLOCK_t* tx_payload;
  };
  osThreadId_t RequestedBy;
} CMD_t;

typedef struct AT_CMD_DEF
{
  ATCommand command;
  ATResponse expected_response;
  uint8_t retries;
  uint32_t timeout_ms;
  uint8_t command_string[16];
} AT_CMD_DEF_t;

typedef struct AT_RESPONSE_DEF
{
  ATResponse response;
  uint8_t response_string[16];
} AT_RESPONSE_DEF_t;

extern osThreadId_t ATTaskHandle;
extern osThreadAttr_t ATTask_attributes;
extern osSemaphoreId_t ATCommandSemaphoreHandle;
extern osMessageQueueId_t ATQueueHandle;
extern osSemaphoreId_t ATResponseSemaphoreHandle;

ATResponse sendRAWAT(uint8_t *cmd);

#endif
