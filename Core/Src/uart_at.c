#include "cmsis_os.h"
#include "uart_at.h"
#include "uartRingBufDMA.h"

#define SEND_RAW_AT_WAIT_FLAG (0x80)

uint8_t cmd_buffer[20];
uint8_t cmd_len = 0;
uint8_t byte = 0;
osMemoryPoolId_t mpid_ATCMD_MemPool; // memory pool id
osMemoryPoolId_t mpid_LoRaPayload_MemPool; // memory pool id
extern osMessageQueueId_t uartQueueHandle;

ATResponse ParseAT(char *buffer);
ATResponse ParseResponse(char *buffer);

const AT_CMD_DEF_t AT_COMMANDS[NUM_AT_COMMANDS] =
    {
        {.command = ATZ, .expected_response = AT_RESET, .retries = 3, .timeout_ms = 300, .command_string = "ATZ"},
        {.command = AT_CFM, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+CFM"},
        {.command = AT_APPKEY, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+APPKEY"},
        {.command = AT_APPEUI, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+APPEUI"},
        {.command = AT_ADR, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+ADR"},
        {.command = AT_NJM, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+NJM"},
        {.command = AT_JOIN, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+JOIN"},
        {.command = AT_CHMASK, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+CHMASK"},
        {.command = AT_SENDB, .expected_response = AT_TX_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+SENDB"},
        {.command = AT_SEND, .expected_response = AT_TX_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT+SEND"},
        {.command = AT, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "AT\r\n"},
        {.command = AT_COMMAND_UNDEFINED, .expected_response = AT_OK, .retries = 3, .timeout_ms = 300, .command_string = "UNDEFINED"}};

const AT_RESPONSE_DEF_t AT_RESPONSES[NUM_AT_RESPONSES] =
    {
        {.response = AT_OK, .response_string = "AT_OK"},
        {.response = AT_TX_OK, .response_string = "AT_TX_OK"},
        {.response = AT_RX_OK, .response_string = "RX:"},
        {.response = AT_ERROR, .response_string = "AT_ERROR"},
        {.response = AT_JOINED, .response_string = "AT_JOIN_OK"},
        {.response = AT_TIMEOUT, .response_string = "AT_TIMEOUT"},
        {.response = AT_BUSY, .response_string = "AT_BUSY"},
        {.response = AT_JOIN_ERROR, .response_string = "AT_JOIN_ERROR"},
        {.response = AT_RESET, .response_string = "Radioenge"},
        {.response = AT_NO_NETWORK_JOINED, .response_string = "AT_NO_NETWORK_JOINED"},
        {.response = AT_RESPONSE_UNDEFINED, .response_string = "UNDEFINED"}};

extern void LoRaWAN_RxEventCallback(uint8_t *data, uint32_t length, uint32_t port, int32_t rssi, int32_t snr);
extern void LoRaWAN_JoinCallback(ATResponse response);

ATResponse gPendingResponse;
UART_MEM_BLOCK_t gATPayload;

ATResponse sendRAWAT(uint8_t *cmd)
{    
    CMD_t* pATCommand;
    osStatus_t ret;
    uint32_t retries = 0;
    osSemaphoreAcquire(ATCommandSemaphoreHandle, osWaitForever);
    pATCommand = (CMD_t *)osMemoryPoolAlloc(mpid_ATCMD_MemPool, 0U); // get Mem Block
    if(pATCommand != NULL)
    {
        pATCommand->command = ParseAT(cmd);
        pATCommand->response = AT_RESPONSE_UNDEFINED;
        if(pATCommand->command != AT_COMMAND_UNDEFINED )
        {
            memcpy(gATPayload.Buf,cmd,strlen(cmd)+1);
            pATCommand->tx_payload = &gATPayload;
            gPendingResponse = AT_RESPONSE_UNDEFINED;
            pATCommand->RequestedBy = osThreadGetId();	
            do
            {
                ret = osMessageQueuePut(ATQueueHandle, &pATCommand, 0U, 250U);
                if (ret != osOK)
                {
                    if(retries++>4)
                    {
                        return AT_ERROR;                
                    }
                }
            }while(ret != osOK);
            //wait for response
            osThreadFlagsClear (SEND_RAW_AT_WAIT_FLAG);            
            osThreadFlagsWait (SEND_RAW_AT_WAIT_FLAG, osFlagsWaitAny, osWaitForever);            
        }
        else
        {
            osMemoryPoolFree(mpid_ATCMD_MemPool,pATCommand);
            gPendingResponse = AT_ERROR;
        }
    }
    else
    {
        gPendingResponse = AT_ERROR;
    }
    
    osSemaphoreRelease(ATCommandSemaphoreHandle);
    return gPendingResponse;
}

ATResponse ParseResponse(char *buffer)
{ 
    for (uint32_t i = 0; i < NUM_AT_RESPONSES; i++)
    {
        if (strncmp(buffer, AT_RESPONSES[i].response_string, strlen(AT_RESPONSES[i].response_string)) == 0)
        {
            return AT_RESPONSES[i].response;
        }
    }
    return AT_RESPONSES[NUM_AT_RESPONSES-1].response; //undefined    
}

ATResponse ParseAT(char *buffer)
{    
    for (uint32_t i = 0; i < NUM_AT_COMMANDS; i++)
    {
        if (strncmp(buffer, AT_COMMANDS[i].command_string, strlen(AT_COMMANDS[i].command_string)) == 0)
        {
            return AT_COMMANDS[i].command;
        }
    }
    return AT_COMMANDS[NUM_AT_COMMANDS - 1].command; // undefined
}

void ATHandlingTaskCode(void *argument)
{
    /* USER CODE BEGIN 5 */
    /* Infinite loop */
    AT_FSM ATTaskFSM = AT_IDLE;
    osStatus_t atevent;    
    CMD_t* new_cmd=NULL;
    CMD_t* send_cmd=NULL;
    CMD_t* PendingCommand = NULL;
    uint32_t CurrentRetries=0;    

    while (1)
    {
        atevent = osMessageQueueGet(ATQueueHandle, &new_cmd, NULL, 3000U);   // wait for message
        if(atevent == osErrorTimeout)
        {
            if(PendingCommand!=NULL)
            {
                if(CurrentRetries < AT_COMMANDS[PendingCommand->command].retries)
                {
                    send_cmd = PendingCommand;
                    CurrentRetries++;
                }
                else
                {                    
                    gPendingResponse = AT_TIMEOUT;
                    osThreadFlagsSet(PendingCommand->RequestedBy, SEND_RAW_AT_WAIT_FLAG);
                    osMemoryPoolFree(mpid_ATCMD_MemPool,PendingCommand);
                    PendingCommand = NULL;                    
                    ATTaskFSM = AT_IDLE;
                }
            }
        }
        else if (atevent == osOK)
        {         
            switch (new_cmd->command)
            {
                case ATZ:
                case AT_CFM:
                case AT_APPKEY:
                case AT_APPEUI:
                case AT_ADR:
                case AT_NJM:
                case AT_JOIN:
                case AT_SENDB:
                case AT_SEND:
                case AT_CHMASK:
                case AT:
                {
                    send_cmd = new_cmd;
                    CurrentRetries = 0;
                    break;
                }    
                case AT_COMMAND_UNDEFINED:        
                default:
                {
                    if(new_cmd->response == AT_RESPONSE_UNDEFINED)
                    {
                        //malformed CMD_t - should not be happening
                        gPendingResponse = AT_ERROR;
                        if(PendingCommand)
                        {
                            osThreadFlagsSet(PendingCommand->RequestedBy, SEND_RAW_AT_WAIT_FLAG);
                            osMemoryPoolFree(mpid_ATCMD_MemPool,PendingCommand);
                        }
                        osMemoryPoolFree(mpid_ATCMD_MemPool,new_cmd);                        
                        PendingCommand = NULL;                        
                        ATTaskFSM = AT_IDLE;
                    }                    
                    break;
                }                
            }
            if (new_cmd->response == AT_RX_OK)
            {
                LoRaWAN_RxEventCallback(new_cmd->rx_payload->Buf, new_cmd->rx_payload->rcvDataLen, new_cmd->rx_payload->rcvPort, new_cmd->rx_payload->rcvRSSI, new_cmd->rx_payload->rcvSNR);
                osMemoryPoolFree(mpid_LoRaPayload_MemPool, PendingCommand->rx_payload);                
                osMemoryPoolFree(mpid_ATCMD_MemPool,new_cmd);                        
            }
            else if ((new_cmd->response == AT_JOINED) || (new_cmd->response == AT_JOIN_ERROR))
            {
                LoRaWAN_JoinCallback(new_cmd->response);
                osMemoryPoolFree(mpid_ATCMD_MemPool, PendingCommand);
                osMemoryPoolFree(mpid_ATCMD_MemPool,new_cmd);                        
            }
            else if (new_cmd->command == AT_COMMAND_UNDEFINED) // it is a response
            {
                switch (ATTaskFSM)
                {
                case AT_IDLE:
                {
                    //unexpected response - ignore
                    if (PendingCommand)
                    {
                        osMemoryPoolFree(mpid_ATCMD_MemPool, PendingCommand);
                    }
                    osMemoryPoolFree(mpid_ATCMD_MemPool,new_cmd);                        
                    break;
                }
                case AT_WAITING_RESPONSE:
                {
                    if (new_cmd->response == AT_COMMANDS[PendingCommand->command].expected_response)
                    {
                        // all good
                        gPendingResponse = AT_COMMANDS[PendingCommand->command].expected_response;
                        if (PendingCommand)
                        {
                            osThreadFlagsSet(PendingCommand->RequestedBy, SEND_RAW_AT_WAIT_FLAG);
                            osMemoryPoolFree(mpid_ATCMD_MemPool, PendingCommand);
                        }
                        osMemoryPoolFree(mpid_ATCMD_MemPool, new_cmd);
                        PendingCommand = NULL;
                        ATTaskFSM = AT_IDLE;                        
                    }
                    else
                    {
                        //unexpected response - retry
                        if (CurrentRetries < AT_COMMANDS[PendingCommand->command].retries)
                        {
                            CurrentRetries++;
                            send_cmd = PendingCommand;
                            PendingCommand = NULL;
                            osMemoryPoolFree(mpid_ATCMD_MemPool, new_cmd);
                        }
                        else
                        {
                            // command failure after retries
                            gPendingResponse = new_cmd->response;
                            if(PendingCommand != NULL)
                            {
                                osThreadFlagsSet(PendingCommand->RequestedBy, SEND_RAW_AT_WAIT_FLAG);
                                osMemoryPoolFree(mpid_ATCMD_MemPool, PendingCommand);
                            }
                            osMemoryPoolFree(mpid_ATCMD_MemPool, new_cmd);
                            PendingCommand = NULL;                            
                            ATTaskFSM = AT_IDLE;                            
                        }
                    }
                    break;
                }
                }
            }            
        }
        if (send_cmd != NULL)
        {
            PendingCommand = send_cmd;
            send_cmd = NULL;
            ATTaskFSM = AT_WAITING_RESPONSE;
            SendToUART(PendingCommand->tx_payload->Buf, strlen(PendingCommand->tx_payload->Buf));
        }
    }
    /* USER CODE END StartCmdProcessing */
    /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartATHandling */
/**
 * @brief Function implementing the cmdHandlingTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartATHandling */
void ATParsingTaskCode(void const *argument)
{
    /* USER CODE BEGIN StartATHandling */
    /* Infinite loop */
    // REF: https://www.keil.com/pack/doc/cmsis/RTOS/html/group__CMSIS__RTOS__Mail.html
    osStatus_t uartevent;
    UART_MEM_BLOCK_t *pMem;
    LORA_PAYLOAD_MEM_BLOCK_t *pLoRaPayload;
    CMD_t *pATResponse;
    
    mpid_ATCMD_MemPool = osMemoryPoolNew(ATCMD_MEMPOOL_OBJECTS, sizeof(CMD_t), NULL);
    mpid_LoRaPayload_MemPool = osMemoryPoolNew(LORAPAYLOAD_MEMPOOL_OBJECTS, sizeof(LORA_PAYLOAD_MEM_BLOCK_t), NULL);
    Ringbuf_Init();
    uint32_t CurrentRetries = 0;
    while (1)
    {
        uartevent = osMessageQueueGet(uartQueueHandle, &pMem, NULL,  osWaitForever); // wait for message
        if (uartevent == osOK)
        {
            pATResponse = (CMD_t *)osMemoryPoolAlloc(mpid_ATCMD_MemPool, 0U); // get Mem Block
            if (pATResponse != NULL)
            {
                pATResponse->command = AT_COMMAND_UNDEFINED;
                pATResponse->response = ParseResponse(pMem->Buf);
                if(pATResponse!=AT_RESPONSE_UNDEFINED)
                {
                    if(pATResponse->response==AT_RX_OK)
                    {
                        //parse received data
                        char *pch;
                        char *rcvDataPointer, asciiChar[3];  
                        uint32_t number;                      
                        pLoRaPayload = (LORA_PAYLOAD_MEM_BLOCK_t *)osMemoryPoolAlloc(mpid_LoRaPayload_MemPool, 0U); // get Mem Block                                               
                        // Data Format: RX:616263:2:-43:25
                        pch = strtok(pMem->Buf, ":");
                        pch = strtok(NULL, ":");
                        rcvDataPointer = pch;
                        asciiChar[3] = '\0';
                        pLoRaPayload->Buf[0] = '\0';
                        pLoRaPayload->rcvDataLen = strlen(rcvDataPointer);
                        for (uint32_t i = 0; i < pLoRaPayload->rcvDataLen; i = i + 2)
                        {
                            memcpy(asciiChar, rcvDataPointer + i, 2);
                            number = (uint32_t)strtol(asciiChar, NULL, 16);
                            //sprintf(pLoRaPayload->Buf, "%s%c", pLoRaPayload->Buf, number);
                            pLoRaPayload->Buf[i / 2] = number & 0x00FF;
                        }
                        pch = strtok(NULL, ":");
                        pLoRaPayload->rcvPort = atoi(pch);
                        pch = strtok(NULL, ":");
                        pLoRaPayload->rcvRSSI = atoi(pch);
                        pch = strtok(NULL, ":");
                        pLoRaPayload->rcvSNR = atoi(pch);
                        pATResponse->rx_payload = pLoRaPayload;
                    }    
                    else
                    {
                        pATResponse->rx_payload = NULL;
                    }                    
                    // Put command to queue
                    osMessageQueuePut(ATQueueHandle, &pATResponse, 0U, 0U);
                }
                else
                {
                    osMemoryPoolFree(mpid_ATCMD_MemPool,pATResponse);                    
                }
            } 
            else
            {
                while(1);
            }           
            osMemoryPoolFree(mpid_UART_MemPool,pMem);
        }        
    }
}
/* USER CODE END StartATHandling */
