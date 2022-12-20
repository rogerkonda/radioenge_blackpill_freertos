#include "cmsis_os.h"
#include "radioenge_modem.h"
#include "uart_at.h"
#include <string.h>

volatile JOINED_STATE gJoinedFSM = JOINED_TX;
volatile RADIO_STATE gRadioState = RADIO_RESET;

extern osThreadId_t ModemMngrTaskHandle;
extern osSemaphoreId_t RadioStateSemaphoreHandle;
extern osMessageQueueId_t ModemSendQueueHandle;
extern osSemaphoreId_t LoRaTXSemaphoreHandle;

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

void LoRaWAN_JoinCallback(ATResponse response)
{
    if (gRadioState == RADIO_JOINING)
    {
        if (response == AT_JOINED)
        {
            SetRadioState(RADIO_JOINED);
        }
        else
        {
            SetRadioState(RADIO_RESET);
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

void ModemManagerTaskCode(void *argument)
{
    /* USER CODE BEGIN 5 */
    /* Infinite loop */    
    uint32_t ConfigCmdIndex = 0;
    uint32_t flags;
    while (1)
    {
        flags = osThreadFlagsWait (0x01, osFlagsWaitAny,osWaitForever);
        switch (gRadioState)
        {
        case RADIO_RESET:
        {
            // CDC_Transmit_FS("Resetting the radio...\r\n", strlen("Resetting the radio...\r\n"));
            resetRadio();
            ConfigCmdIndex = 0;
            SetRadioState(RADIO_CONFIGURING);
            osThreadFlagsSet(ModemMngrTaskHandle, 0x01);
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
            /* USER CODE END StartCmdProcessing */
            /* USER CODE END 5 */
        }
    }
}

#define OUT_BUFFER_SIZE (64)
uint8_t gEncodedString[OUT_BUFFER_SIZE]; 
uint8_t gSendBuffer[OUT_BUFFER_SIZE+16]; 


void LoRaSend(uint32_t LoraWANPort,uint8_t* msg)
{    
    osSemaphoreAcquire(LoRaTXSemaphoreHandle,osWaitForever);
    LoRaSendNow(LoraWANPort,msg);    
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

void LoRaSendB(uint32_t LoraWANPort, uint8_t* msg, size_t size)
{
    osSemaphoreAcquire(LoRaTXSemaphoreHandle,osWaitForever);
    LoRaSendBNow(LoraWANPort,msg,size);    
}


//LoRaSendNow and LoRaSendBNow should only be used in conjunction with LoRaWaitDutyCycle()

void LoRaWaitDutyCycle()
{
    osSemaphoreAcquire(LoRaTXSemaphoreHandle,osWaitForever);
}

void LoRaSendNow(uint32_t LoraWANPort, uint8_t* msg)
{
    snprintf(gSendBuffer, OUT_BUFFER_SIZE+16, "AT+SEND=%d:%s\r\n", LoraWANPort, msg);
    if(sendRAWAT(gSendBuffer)!=AT_OK)
    {
        osThreadFlagsSet(ModemMngrTaskHandle, 0x01);            
    }
    else
    {
        osSemaphoreRelease(LoRaTXSemaphoreHandle);
    }
}

void LoRaSendBNow(uint32_t LoraWANPort, uint8_t* msg, size_t size)
{
    size_t encoded_size = bin_encode((void *)(msg), size, gEncodedString, OUT_BUFFER_SIZE);
    snprintf(gSendBuffer, OUT_BUFFER_SIZE+16, "AT+SENDB=%d:%s\r\n", LoraWANPort, gEncodedString);
    if(sendRAWAT(gSendBuffer)!=AT_OK)
    {
        osThreadFlagsSet(ModemMngrTaskHandle, 0x01);            
    }
    else
    {
        osSemaphoreRelease(LoRaTXSemaphoreHandle);
    }
}


void ModemSendTaskCode(void const *argument)
{
    uint32_t DutyCycle_ms = *(uint32_t*)argument;
    uint32_t flags;    
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
