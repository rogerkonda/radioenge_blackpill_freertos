#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "radioenge_modem.h"

extern osTimerId_t PeriodicSendTimerHandle;
extern osThreadId_t AppSendTaskHandle;
extern ADC_HandleTypeDef hadc1;
extern osEventFlagsId_t ModemStatusFlagsHandle;


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
    uint32_t read;
    int32_t temp;
    uint32_t modemflags;
    osTimerStart(PeriodicSendTimerHandle, 30000U);

    while (1)
    {        
        LoRaWaitDutyCycle();
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);
        read = HAL_ADC_GetValue(&hadc1);
        temp = (int32_t)(330 * ((float)read / 4096));
        LoRaSendBNow(2, (uint8_t *)&temp, sizeof(int32_t));
        osThreadFlagsClear(0x01);
        osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);        
    }
}
