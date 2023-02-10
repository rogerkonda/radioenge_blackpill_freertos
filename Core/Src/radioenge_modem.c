#include "cmsis_os.h"
#include "radioenge_modem.h"
#include "uart_at.h"
#include "main.h"
#include <string.h>

volatile JOINED_STATE gJoinedFSM = JOINED_TX;
volatile RADIO_STATE gRadioState = RADIO_RESET;

extern osThreadId_t ModemMngrTaskHandle;
extern osSemaphoreId_t RadioStateSemaphoreHandle;
extern osMessageQueueId_t ModemSendQueueHandle;
extern osSemaphoreId_t LoRaTXSemaphoreHandle;
extern osEventFlagsId_t ModemStatusFlagsHandle;
extern osTimerId_t ModemLedTimerHandle;
extern osMessageQueueId_t uartQueueHandle;

#define NUMBER_OF_STRINGS (7)
#define STRING_LENGTH (255)
char gConfigCmds[NUMBER_OF_STRINGS][STRING_LENGTH + 1] = {
    "AT+CFM=0\r\n",
    "AT+APPKEY=00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF\r\n",
    "AT+APPEUI=00:00:00:00:00:00:00:00\r\n",
    "AT+CHMASK=ff00:0000:0000:0000:0002:0000\r\n",
    "AT+ADR=1\r\n",
    "AT+NJM=1\r\n",
    "AT+JOIN\r\n"};


uint32_t gConsecutiveJoinErrors = 0;

void LoRaWAN_JoinCallback(ATResponse response)
{
    if (gRadioState == RADIO_JOINING)
    {
        if (response == AT_JOINED)
        {
            gConsecutiveJoinErrors = 0;
            SetRadioState(RADIO_JOINED);
        }
        else
        {
            gConsecutiveJoinErrors++;
            if(gConsecutiveJoinErrors==9) //radioenge modem stops after 9 join errors
            {
                SetRadioState(RADIO_RESET);
            }
        }
        osThreadFlagsSet(ModemMngrTaskHandle, 0x01);
    }
}

void resetRadio(void)
{
    while (sendRAWAT("ATZ\r\n") != AT_RESET)
    {
        osDelay(500);
    }
    return;
}

void SetRadioState(RADIO_STATE state)
{
    osSemaphoreAcquire(RadioStateSemaphoreHandle, osWaitForever);
    gRadioState = state;
    osSemaphoreRelease(RadioStateSemaphoreHandle);
}

void ModemLedCallback(void *argument) 
{
    switch(gRadioState)
    {
    case RADIO_RESET:
    {
        HAL_GPIO_TogglePin(LED1_RED_GPIO_Port, LED1_RED_Pin);
        HAL_GPIO_WritePin(LED2_YELLOW_GPIO_Port, LED2_YELLOW_Pin, 0);
        HAL_GPIO_WritePin(LED3_GREEN_GPIO_Port, LED3_GREEN_Pin, 0);
        HAL_GPIO_WritePin(LED4_WHITE_GPIO_Port, LED4_WHITE_Pin, 0); 
        break;       
    }
    case RADIO_CONFIGURING:
    {
        HAL_GPIO_WritePin(LED1_RED_GPIO_Port, LED1_RED_Pin,0);
        HAL_GPIO_TogglePin(LED2_YELLOW_GPIO_Port, LED2_YELLOW_Pin);
        HAL_GPIO_WritePin(LED3_GREEN_GPIO_Port, LED3_GREEN_Pin,0);
        HAL_GPIO_WritePin(LED4_WHITE_GPIO_Port, LED4_WHITE_Pin, 0);        
        break;       
    }
    case RADIO_JOINING:
    {
        HAL_GPIO_WritePin(LED1_RED_GPIO_Port, LED1_RED_Pin,0);
        HAL_GPIO_WritePin(LED2_YELLOW_GPIO_Port, LED2_YELLOW_Pin,0);
        HAL_GPIO_TogglePin(LED3_GREEN_GPIO_Port, LED3_GREEN_Pin);
        HAL_GPIO_WritePin(LED4_WHITE_GPIO_Port, LED4_WHITE_Pin, 0);        
        break;       
    }
    case RADIO_JOINED:
    {
        HAL_GPIO_WritePin(LED1_RED_GPIO_Port, LED1_RED_Pin,0);
        HAL_GPIO_WritePin(LED2_YELLOW_GPIO_Port, LED2_YELLOW_Pin,0);
        HAL_GPIO_WritePin(LED3_GREEN_GPIO_Port, LED3_GREEN_Pin,1);
        HAL_GPIO_WritePin(LED4_WHITE_GPIO_Port, LED4_WHITE_Pin, 0);        
        break;       
    }
    default:
    {
        HAL_GPIO_TogglePin(LED1_RED_GPIO_Port, LED1_RED_Pin);
        HAL_GPIO_TogglePin(LED2_YELLOW_GPIO_Port, LED2_YELLOW_Pin);
        HAL_GPIO_TogglePin(LED3_GREEN_GPIO_Port, LED3_GREEN_Pin);
        HAL_GPIO_TogglePin(LED4_WHITE_GPIO_Port, LED4_WHITE_Pin);        
        break;       
    }    
    }
}




void ModemManagerTaskCode(void *argument)
{
    /* USER CODE BEGIN 5 */
    /* Infinite loop */    
    uint32_t ConfigCmdIndex = 0;
    uint32_t flags;
    uint32_t modemflags;
    osTimerStart(ModemLedTimerHandle, 50U);
    osThreadFlagsSet(ModemMngrTaskHandle, 0x01);
    
    while (1)
    {
        flags = osThreadFlagsWait (0x01, osFlagsWaitAny,osWaitForever);
        osEventFlagsClear(ModemStatusFlagsHandle, RADIO_STATE_ALL);        
        switch (gRadioState)
        {
        case RADIO_RESET:
        {
            // CDC_Transmit_FS("Resetting the radio...\r\n", strlen("Resetting the radio...\r\n"));
            resetRadio();
            ConfigCmdIndex = 0;
            SetRadioState(RADIO_CONFIGURING);
            osThreadFlagsSet(ModemMngrTaskHandle, 0x01);
            osDelay(1000);
            break;
        }
        case RADIO_CONFIGURING:
        {
            if (sendRAWAT(gConfigCmds[ConfigCmdIndex]) == AT_OK)
            {                
                ConfigCmdIndex++;
                if (ConfigCmdIndex == NUMBER_OF_STRINGS)
                {
                    SetRadioState(RADIO_JOINING);
                }
                osDelay(100);
            }
            else
            {
                SetRadioState(RADIO_RESET);
            }
            osThreadFlagsSet(ModemMngrTaskHandle, 0x01);
            break;
        }
        case RADIO_JOINING:
        {
            // wait for radio callback
            break;
        }
        case RADIO_JOINED:
        {
            // now the send thread can work
            break;
        }
        }
        modemflags = osEventFlagsSet(ModemStatusFlagsHandle, gRadioState);
    }
}

#define OUT_BUFFER_SIZE (64)
uint8_t gEncodedString[OUT_BUFFER_SIZE]; 
uint8_t gSendBuffer[OUT_BUFFER_SIZE+16]; 


osStatus_t LoRaSend(uint32_t LoraWANPort,uint8_t* msg)
{    
    osSemaphoreAcquire(LoRaTXSemaphoreHandle,osWaitForever);
    return LoRaSendNow(LoraWANPort,msg);    
}

size_t bin_encode(void* in, size_t in_size, uint8_t* out, size_t max_out_size)
{
    uint8_t* in_ptr = (uint8_t*)in;
    size_t offset=0;
    size_t iter = in_size;
    size_t i;
    //check for buffer overflow
    if(max_out_size<in_size*2)
    {
        iter = max_out_size/2;
    }
    else
    {
        iter = in_size;
    }
    //create the hex string expected by RadioEnge AT
    for(i=0;i<iter;i++)
    {
        sprintf(out+offset,"%02x",*(in_ptr++));
        offset+=2*sizeof(uint8_t);
    }
    return offset; //returns the size of the AT buffer
}

osStatus_t LoRaSendB(uint32_t LoraWANPort, uint8_t* msg, size_t size)
{
    osSemaphoreAcquire(LoRaTXSemaphoreHandle,osWaitForever);
    return LoRaSendBNow(LoraWANPort,msg,size);    
}


//LoRaSendNow and LoRaSendBNow should only be used in conjunction with LoRaWaitDutyCycle()

void LoRaWaitDutyCycle()
{
    osSemaphoreAcquire(LoRaTXSemaphoreHandle,osWaitForever);
}

osStatus_t LoRaSendNow(uint32_t LoraWANPort, uint8_t* msg)
{
    if (gRadioState == RADIO_JOINED)
    {
        snprintf(gSendBuffer, OUT_BUFFER_SIZE + 16, "AT+SEND=%d:%s\r\n", LoraWANPort, msg);
        if (sendRAWAT(gSendBuffer) != AT_OK)
        {
            osThreadFlagsSet(ModemMngrTaskHandle, 0x01);
            return osOK;
        }
        else
        {
            osSemaphoreRelease(LoRaTXSemaphoreHandle);
        }
    }
    return osError;
}

osStatus_t LoRaSendBNow(uint32_t LoraWANPort, uint8_t* msg, size_t size)
{
    if (gRadioState == RADIO_JOINED)
    {
        size_t encoded_size = bin_encode((void *)(msg), size, gEncodedString, OUT_BUFFER_SIZE);
        snprintf(gSendBuffer, OUT_BUFFER_SIZE + 16, "AT+SENDB=%d:%s\r\n", LoraWANPort, gEncodedString);
        if (sendRAWAT(gSendBuffer) != AT_OK)
        {
            osThreadFlagsSet(ModemMngrTaskHandle, 0x01);
            return osOK;
        }
        else
        {
            osSemaphoreRelease(LoRaTXSemaphoreHandle);
        }
    }
    return osError;
}


void ModemSendTaskCode(void const *argument)
{
    uint32_t DutyCycle_ms = *(uint32_t*)argument;
    uint32_t flags;    
    osSemaphoreAcquire(LoRaTXSemaphoreHandle,osWaitForever);
    while (1)
    {
        flags = osThreadFlagsWait (0x01, osFlagsWaitAny,osWaitForever);        
        {
            osDelay	(DutyCycle_ms);	
            osSemaphoreRelease(LoRaTXSemaphoreHandle);    
        }        
    }
}
/* USER CODE END StartATHandling */
