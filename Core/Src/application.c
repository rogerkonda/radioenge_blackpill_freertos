#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

extern osTimerId_t PeriodicSendTimerHandle;
extern osThreadId_t AppSendTaskHandle;
extern ADC_HandleTypeDef hadc1;


void LoRaWAN_RxEventCallback(uint8_t *data, uint32_t length, uint32_t port, int32_t rssi, int32_t snr)
{

}


void PeriodicSendTimerCallback (void *argument) 
{
    osThreadFlagsSet(AppSendTaskHandle, 0x01);
}


void AppSendTaskCode(void *argument)
{
    /* USER CODE BEGIN 5 */
    /* Infinite loop */    
    osTimerStart(PeriodicSendTimerHandle, 30000U);
    uint32_t read;
    int32_t temp;

    while (1)
    {
        osThreadFlagsWait (0x01, osFlagsWaitAny,osWaitForever);
        LoRaWaitDutyCycle();
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1,100);
        read = HAL_ADC_GetValue(&hadc1);
        temp = (int32_t)(100*((float)read/4096));
        LoRaSendBNow(2, (uint8_t*)&temp,sizeof(int32_t));
    }        
}

