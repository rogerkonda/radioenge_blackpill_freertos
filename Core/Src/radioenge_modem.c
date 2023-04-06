#include "cmsis_os.h"
#include "radioenge_modem.h"
#include "uart_at.h"
#include "main.h"
#include <string.h>

volatile JOINED_STATE gJoinedFSM = JOINED_TX;
volatile RADIO_STATE gRadioState = RADIO_RESET;

extern osThreadId_t ModemMngrTaskHandle;
extern osThreadId_t ModemSendTaskHandle;
extern osSemaphoreId_t RadioStateSemaphoreHandle;
extern osMessageQueueId_t ModemSendQueueHandle;
extern osSemaphoreId_t LoRaTXSemaphoreHandle;
extern osEventFlagsId_t ModemStatusFlagsHandle;
extern osTimerId_t ModemLedTimerHandle;
extern osMessageQueueId_t uartQueueHandle;
extern osTimerId_t DutyCycleTimerHandle;

#define NUMBER_OF_STRINGS (2)
#define STRING_LENGTH (255)
char gConfigCmds[NUMBER_OF_STRINGS][STRING_LENGTH + 1] = {
    "AT\r\n",
    "AT\r\n"
    };


uint32_t gConsecutiveJoinErrors = 0;

void LoRaWAN_JoinCallback(ATResponse response)
{
    osSemaphoreAcquire(RadioStateSemaphoreHandle, osWaitForever);
    if (gRadioState == RADIO_JOINING)
    {
        if (response == AT_JOINED)
        {
            gConsecutiveJoinErrors = 0;
            SetRadioState(RADIO_READY);
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
    osSemaphoreRelease(RadioStateSemaphoreHandle);
}

void DutyCycleTimerCallback (void *argument) 
{
    osSemaphoreAcquire(RadioStateSemaphoreHandle, osWaitForever);
    if(gRadioState==RADIO_DUTYCYCLED)
    {
        SetRadioState(RADIO_READY);
    }  
    osSemaphoreRelease(RadioStateSemaphoreHandle);  
}

void resetRadio(void)
{
    while (sendRAWAT("ATZ\r\n") != AT_RESET)
    {
        osDelay(5000);
    }
    return;
}

//calls to this function must be protected by semaphore RadioStateSemaphoreHandle
void SetRadioState(RADIO_STATE state)
{    
    gRadioState = state;
    osThreadFlagsSet(ModemMngrTaskHandle, 0x01); 
}

void ModemLedCallback(void *argument) 
{
    //here we use gRadioState without semaphore because a preemption will only cause a momentary led glitch    
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
    case RADIO_READY:
    {
        HAL_GPIO_WritePin(LED1_RED_GPIO_Port, LED1_RED_Pin,0);
        HAL_GPIO_WritePin(LED2_YELLOW_GPIO_Port, LED2_YELLOW_Pin,0);
        HAL_GPIO_WritePin(LED3_GREEN_GPIO_Port, LED3_GREEN_Pin,1);
        HAL_GPIO_WritePin(LED4_WHITE_GPIO_Port, LED4_WHITE_Pin, 0);        
        break;       
    }
    case RADIO_DUTYCYCLED:
    {
        HAL_GPIO_WritePin(LED1_RED_GPIO_Port, LED1_RED_Pin,0);
        HAL_GPIO_WritePin(LED2_YELLOW_GPIO_Port, LED2_YELLOW_Pin,0);
        HAL_GPIO_WritePin(LED3_GREEN_GPIO_Port, LED3_GREEN_Pin,1);
        HAL_GPIO_TogglePin(LED4_WHITE_GPIO_Port, LED4_WHITE_Pin);        
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
        osSemaphoreAcquire(RadioStateSemaphoreHandle, osWaitForever);
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
        case RADIO_READY:
        {
            // now we can send data
            break;
        }
        case RADIO_DUTYCYCLED:
        {
            // Wait the dutycycle period and return to ready
            osTimerStart(DutyCycleTimerHandle, 14000U);
            break;
        }
        }
        modemflags = osEventFlagsSet(ModemStatusFlagsHandle, gRadioState);
        osSemaphoreRelease(RadioStateSemaphoreHandle);
    }
}

#define OUT_BUFFER_SIZE (64)
uint8_t gEncodedString[OUT_BUFFER_SIZE]; 
uint8_t gSendBuffer[OUT_BUFFER_SIZE+16]; 


osStatus_t LoRaSend(uint32_t LoraWANPort,uint8_t* msg)
{   
    LoRaWaitDutyCycle();
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
    LoRaWaitDutyCycle();     
    return LoRaSendBNow(LoraWANPort,msg,size);    
}


//LoRaSendNow and LoRaSendBNow should only be used in conjunction with LoRaWaitDutyCycle()

void LoRaWaitDutyCycle()
{
   osEventFlagsWait(ModemStatusFlagsHandle, RADIO_READY, osFlagsWaitAny | osFlagsNoClear, osWaitForever);
   osSemaphoreAcquire(LoRaTXSemaphoreHandle,osWaitForever);    
   osEventFlagsWait(ModemStatusFlagsHandle, RADIO_READY, osFlagsWaitAny | osFlagsNoClear, osWaitForever);
}

osStatus_t LoRaSendNow(uint32_t LoraWANPort, uint8_t* msg)
{
    osStatus_t ret = osError;    
    osSemaphoreAcquire(RadioStateSemaphoreHandle, osWaitForever);
    if (gRadioState == RADIO_READY)
    {
        snprintf(gSendBuffer, OUT_BUFFER_SIZE + 16, "AT+SEND=%d:%s\r\n", LoraWANPort, msg);
        if (sendRAWAT(gSendBuffer) == AT_TX_OK)
        {
            SetRadioState(RADIO_DUTYCYCLED);
            ret = osOK;
        }        
    }    
    osSemaphoreRelease(RadioStateSemaphoreHandle);
    osSemaphoreRelease(LoRaTXSemaphoreHandle);
    return ret;
}

osStatus_t LoRaSendBNow(uint32_t LoraWANPort, uint8_t* msg, size_t size)
{
    osStatus_t ret = osError;
    osSemaphoreAcquire(RadioStateSemaphoreHandle, osWaitForever);
    if (gRadioState == RADIO_READY)
    {
        size_t encoded_size = bin_encode((void *)(msg), size, gEncodedString, OUT_BUFFER_SIZE);
        snprintf(gSendBuffer, OUT_BUFFER_SIZE + 16, "AT+SENDB=%d:%s\r\n", LoraWANPort, gEncodedString);
        if (sendRAWAT(gSendBuffer) == AT_TX_OK)
        {
            SetRadioState(RADIO_DUTYCYCLED);
            ret = osOK;
        }
    }
    osSemaphoreRelease(RadioStateSemaphoreHandle);
    osSemaphoreRelease(LoRaTXSemaphoreHandle);
    return ret;
}
/* USER CODE END StartATHandling */
