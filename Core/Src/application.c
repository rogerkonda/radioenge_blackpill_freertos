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


void AppSendTaskCode(void *argument)
{
    /* USER CODE BEGIN 5 */
    /* Infinite loop */    
    

    while (1)
    {        
        LoRaWaitDutyCycle();       
        //write code to read from sensors and send via LoRaWAN

        osDelay(30000);
    }
}
