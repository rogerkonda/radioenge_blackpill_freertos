#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "radioenge_modem.h"
#include "main.h"
extern osTimerId_t PeriodicSendTimerHandle;
extern osThreadId_t AppSendTaskHandle;
extern ADC_HandleTypeDef hadc1;
extern osEventFlagsId_t ModemStatusFlagsHandle;
extern TIM_HandleTypeDef htim3;


void LoRaWAN_RxEventCallback(uint8_t *data, uint32_t length, uint32_t port, int32_t rssi, int32_t snr)
{

}

void PeriodicSendTimerCallback(void *argument)
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
void BlueLedTaskCode(void *argument)
{
    while(1)
    {
        HAL_GPIO_TogglePin(LED4_BLUE_GPIO_Port, LED4_BLUE_Pin);
        osDelay(50);

    }
}
void RedLedTaskCode(void *argument)
{
    while(1)
    {
        HAL_GPIO_TogglePin(LED1_RED_GPIO_Port, LED1_RED_Pin);
        osDelay(500);

    }
}
void YellowLedTaskCode(void *argument)
{
    while(1)
    {
        HAL_GPIO_TogglePin(LED2_YELLOW_GPIO_Port, LED2_YELLOW_Pin);
        osDelay(800);

    }
}
void GreenLedTaskCode(void *argument)
{
    while(1)
    {
        HAL_GPIO_TogglePin(LED3_GREEN_GPIO_Port, LED3_GREEN_Pin);
        osDelay(250);

    }
}