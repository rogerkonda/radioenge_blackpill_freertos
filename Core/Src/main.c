/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticTimer_t osStaticTimerDef_t;
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
uint32_t defaultTaskBuffer[ 128 ];
osStaticThreadDef_t defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .cb_mem = &defaultTaskControlBlock,
  .cb_size = sizeof(defaultTaskControlBlock),
  .stack_mem = &defaultTaskBuffer[0],
  .stack_size = sizeof(defaultTaskBuffer),
  .priority = (osPriority_t) osPriorityLow1,
};
/* Definitions for ATParsingTask */
osThreadId_t ATParsingTaskHandle;
uint32_t ATParsingTaskBuffer[ 512 ];
osStaticThreadDef_t ATParsingTaskControlBlock;
const osThreadAttr_t ATParsingTask_attributes = {
  .name = "ATParsingTask",
  .cb_mem = &ATParsingTaskControlBlock,
  .cb_size = sizeof(ATParsingTaskControlBlock),
  .stack_mem = &ATParsingTaskBuffer[0],
  .stack_size = sizeof(ATParsingTaskBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal7,
};
/* Definitions for ATHandlingTask */
osThreadId_t ATHandlingTaskHandle;
uint32_t ATHandlingTaskBuffer[ 512 ];
osStaticThreadDef_t ATHandlingTaskControlBlock;
const osThreadAttr_t ATHandlingTask_attributes = {
  .name = "ATHandlingTask",
  .cb_mem = &ATHandlingTaskControlBlock,
  .cb_size = sizeof(ATHandlingTaskControlBlock),
  .stack_mem = &ATHandlingTaskBuffer[0],
  .stack_size = sizeof(ATHandlingTaskBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal6,
};
/* Definitions for UARTProcTask */
osThreadId_t UARTProcTaskHandle;
uint32_t UARTProcTaskBuffer[ 512 ];
osStaticThreadDef_t UARTProcTaskControlBlock;
const osThreadAttr_t UARTProcTask_attributes = {
  .name = "UARTProcTask",
  .cb_mem = &UARTProcTaskControlBlock,
  .cb_size = sizeof(UARTProcTaskControlBlock),
  .stack_mem = &UARTProcTaskBuffer[0],
  .stack_size = sizeof(UARTProcTaskBuffer),
  .priority = (osPriority_t) osPriorityHigh7,
};
/* Definitions for ModemMngrTask */
osThreadId_t ModemMngrTaskHandle;
uint32_t ModemMngrTaskBuffer[ 512 ];
osStaticThreadDef_t ModemMngrTaskControlBlock;
const osThreadAttr_t ModemMngrTask_attributes = {
  .name = "ModemMngrTask",
  .cb_mem = &ModemMngrTaskControlBlock,
  .cb_size = sizeof(ModemMngrTaskControlBlock),
  .stack_mem = &ModemMngrTaskBuffer[0],
  .stack_size = sizeof(ModemMngrTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal7,
};
/* Definitions for AppSendTask */
osThreadId_t AppSendTaskHandle;
uint32_t SendTemperatureBuffer[ 512 ];
osStaticThreadDef_t SendTemperatureControlBlock;
const osThreadAttr_t AppSendTask_attributes = {
  .name = "AppSendTask",
  .cb_mem = &SendTemperatureControlBlock,
  .cb_size = sizeof(SendTemperatureControlBlock),
  .stack_mem = &SendTemperatureBuffer[0],
  .stack_size = sizeof(SendTemperatureBuffer),
  .priority = (osPriority_t) osPriorityLow7,
};
/* Definitions for uartQueue */
osMessageQueueId_t uartQueueHandle;
uint8_t uartQueueBuffer[ 4 * sizeof( void* ) ];
osStaticMessageQDef_t uartQueueControlBlock;
const osMessageQueueAttr_t uartQueue_attributes = {
  .name = "uartQueue",
  .cb_mem = &uartQueueControlBlock,
  .cb_size = sizeof(uartQueueControlBlock),
  .mq_mem = &uartQueueBuffer,
  .mq_size = sizeof(uartQueueBuffer)
};
/* Definitions for ATQueue */
osMessageQueueId_t ATQueueHandle;
uint8_t ATQueueBuffer[ 4 * sizeof( void* ) ];
osStaticMessageQDef_t ATQueueControlBlock;
const osMessageQueueAttr_t ATQueue_attributes = {
  .name = "ATQueue",
  .cb_mem = &ATQueueControlBlock,
  .cb_size = sizeof(ATQueueControlBlock),
  .mq_mem = &ATQueueBuffer,
  .mq_size = sizeof(ATQueueBuffer)
};
/* Definitions for ModemSendQueue */
osMessageQueueId_t ModemSendQueueHandle;
uint8_t ModemSendQueueBuffer[ 4 * sizeof( void* ) ];
osStaticMessageQDef_t ModemSendQueueControlBlock;
const osMessageQueueAttr_t ModemSendQueue_attributes = {
  .name = "ModemSendQueue",
  .cb_mem = &ModemSendQueueControlBlock,
  .cb_size = sizeof(ModemSendQueueControlBlock),
  .mq_mem = &ModemSendQueueBuffer,
  .mq_size = sizeof(ModemSendQueueBuffer)
};
/* Definitions for PeriodicSendTimer */
osTimerId_t PeriodicSendTimerHandle;
osStaticTimerDef_t PeriodicSendTimerControlBlock;
const osTimerAttr_t PeriodicSendTimer_attributes = {
  .name = "PeriodicSendTimer",
  .cb_mem = &PeriodicSendTimerControlBlock,
  .cb_size = sizeof(PeriodicSendTimerControlBlock),
};
/* Definitions for ModemLedTimer */
osTimerId_t ModemLedTimerHandle;
osStaticTimerDef_t ModemLedTimerControlBlock;
const osTimerAttr_t ModemLedTimer_attributes = {
  .name = "ModemLedTimer",
  .cb_mem = &ModemLedTimerControlBlock,
  .cb_size = sizeof(ModemLedTimerControlBlock),
};
/* Definitions for DutyCycleTimer */
osTimerId_t DutyCycleTimerHandle;
osStaticTimerDef_t DutyCycleTimerControlBlock;
const osTimerAttr_t DutyCycleTimer_attributes = {
  .name = "DutyCycleTimer",
  .cb_mem = &DutyCycleTimerControlBlock,
  .cb_size = sizeof(DutyCycleTimerControlBlock),
};
/* Definitions for ATCommandSemaphore */
osSemaphoreId_t ATCommandSemaphoreHandle;
osStaticSemaphoreDef_t ATCommandSemaphoreControlBlock;
const osSemaphoreAttr_t ATCommandSemaphore_attributes = {
  .name = "ATCommandSemaphore",
  .cb_mem = &ATCommandSemaphoreControlBlock,
  .cb_size = sizeof(ATCommandSemaphoreControlBlock),
};
/* Definitions for ATResponseSemaphore */
osSemaphoreId_t ATResponseSemaphoreHandle;
osStaticSemaphoreDef_t ATResponseSemaphoreControlBlock;
const osSemaphoreAttr_t ATResponseSemaphore_attributes = {
  .name = "ATResponseSemaphore",
  .cb_mem = &ATResponseSemaphoreControlBlock,
  .cb_size = sizeof(ATResponseSemaphoreControlBlock),
};
/* Definitions for UARTTXSemaphore */
osSemaphoreId_t UARTTXSemaphoreHandle;
osStaticSemaphoreDef_t UARTTXSemaphoreControlBlock;
const osSemaphoreAttr_t UARTTXSemaphore_attributes = {
  .name = "UARTTXSemaphore",
  .cb_mem = &UARTTXSemaphoreControlBlock,
  .cb_size = sizeof(UARTTXSemaphoreControlBlock),
};
/* Definitions for RadioStateSemaphore */
osSemaphoreId_t RadioStateSemaphoreHandle;
osStaticSemaphoreDef_t RadioStateSemaphoreControlBlock;
const osSemaphoreAttr_t RadioStateSemaphore_attributes = {
  .name = "RadioStateSemaphore",
  .cb_mem = &RadioStateSemaphoreControlBlock,
  .cb_size = sizeof(RadioStateSemaphoreControlBlock),
};
/* Definitions for LoRaTXSemaphore */
osSemaphoreId_t LoRaTXSemaphoreHandle;
osStaticSemaphoreDef_t LoRaTXSemaphoreControlBlock;
const osSemaphoreAttr_t LoRaTXSemaphore_attributes = {
  .name = "LoRaTXSemaphore",
  .cb_mem = &LoRaTXSemaphoreControlBlock,
  .cb_size = sizeof(LoRaTXSemaphoreControlBlock),
};
/* Definitions for ModemStatusFlags */
osEventFlagsId_t ModemStatusFlagsHandle;
osStaticEventGroupDef_t ModemStatusFlagsControlBlock;
const osEventFlagsAttr_t ModemStatusFlags_attributes = {
  .name = "ModemStatusFlags",
  .cb_mem = &ModemStatusFlagsControlBlock,
  .cb_size = sizeof(ModemStatusFlagsControlBlock),
};
/* USER CODE BEGIN PV */
volatile float vector[255];
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
void StartDefaultTask(void *argument);
extern void ATParsingTaskCode(void *argument);
extern void ATHandlingTaskCode(void *argument);
extern void UARTProcTaskCode(void *argument);
extern void ModemManagerTaskCode(void *argument);
extern void AppSendTaskCode(void *argument);
extern void PeriodicSendTimerCallback(void *argument);
extern void ModemLedCallback(void *argument);
extern void DutyCycleTimerCallback(void *argument);

/* USER CODE BEGIN PFP */
void CONFIGURE_TIMER_FOR_RUN_TIME_STATS()
{
  HAL_TIM_Base_Start(&htim2); /* Define this to initialize your timer/counter */
}

uint32_t GET_RUN_TIME_COUNTER_VALUE()
{
  return __HAL_TIM_GET_COUNTER(&htim2); /* Define this to sample the timer/counter */
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
const volatile int uxTopUsedPriority = configMAX_PRIORITIES - 1; //this declaration enables thread awareness for FreeRTOS using OpenOCD

// Paste this code to Core/Inc/FreeRTOSConfig.h to enable kernel statistics (will be overwritten by STMCubeMX everytime code is generated)
// #define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() CONFIGURE_TIMER_FOR_RUN_TIME_STATS()/* Define this to initialize your timer/counter */
// #define portGET_RUN_TIME_COUNTER_VALUE() GET_RUN_TIME_COUNTER_VALUE()        /* Define this to sample the timer/counter */
// #define configGENERATE_RUN_TIME_STATS (1)
// #define configRECORD_STACK_HIGH_ADDRESS (1)

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint32_t i;  
  (void)uxTopUsedPriority; //this declaration enables thread awareness for FreeRTOS using OpenOCD
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  
  //H-Bridge PWMs
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of ATCommandSemaphore */
  ATCommandSemaphoreHandle = osSemaphoreNew(1, 1, &ATCommandSemaphore_attributes);

  /* creation of ATResponseSemaphore */
  ATResponseSemaphoreHandle = osSemaphoreNew(1, 1, &ATResponseSemaphore_attributes);

  /* creation of UARTTXSemaphore */
  UARTTXSemaphoreHandle = osSemaphoreNew(1, 1, &UARTTXSemaphore_attributes);

  /* creation of RadioStateSemaphore */
  RadioStateSemaphoreHandle = osSemaphoreNew(1, 1, &RadioStateSemaphore_attributes);

  /* creation of LoRaTXSemaphore */
  LoRaTXSemaphoreHandle = osSemaphoreNew(1, 1, &LoRaTXSemaphore_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of PeriodicSendTimer */
  PeriodicSendTimerHandle = osTimerNew(PeriodicSendTimerCallback, osTimerPeriodic, NULL, &PeriodicSendTimer_attributes);

  /* creation of ModemLedTimer */
  ModemLedTimerHandle = osTimerNew(ModemLedCallback, osTimerPeriodic, NULL, &ModemLedTimer_attributes);

  /* creation of DutyCycleTimer */
  DutyCycleTimerHandle = osTimerNew(DutyCycleTimerCallback, osTimerOnce, NULL, &DutyCycleTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of uartQueue */
  uartQueueHandle = osMessageQueueNew (4, sizeof(void*), &uartQueue_attributes);

  /* creation of ATQueue */
  ATQueueHandle = osMessageQueueNew (4, sizeof(void*), &ATQueue_attributes);

  /* creation of ModemSendQueue */
  ModemSendQueueHandle = osMessageQueueNew (4, sizeof(void*), &ModemSendQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of ATParsingTask */
  ATParsingTaskHandle = osThreadNew(ATParsingTaskCode, NULL, &ATParsingTask_attributes);

  /* creation of ATHandlingTask */
  ATHandlingTaskHandle = osThreadNew(ATHandlingTaskCode, NULL, &ATHandlingTask_attributes);

  /* creation of UARTProcTask */
  UARTProcTaskHandle = osThreadNew(UARTProcTaskCode, NULL, &UARTProcTask_attributes);

  /* creation of ModemMngrTask */
  ModemMngrTaskHandle = osThreadNew(ModemManagerTaskCode, NULL, &ModemMngrTask_attributes);

  /* creation of AppSendTask */
  AppSendTaskHandle = osThreadNew(AppSendTaskCode, NULL, &AppSendTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of ModemStatusFlags */
  ModemStatusFlagsHandle = osEventFlagsNew(&ModemStatusFlags_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();
  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    for (i = 0; i < 8; i++)
    {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 0);
      HAL_Delay(25);
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);
      HAL_Delay(50);
    }
    HAL_Delay(800);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 9600-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 9600-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 9600-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 9600-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 9600-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED3_GREEN_Pin|LED1_RED_Pin|LED4_WHITE_Pin|LED2_YELLOW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BRIDGE_1_EN_GPIO_Port, BRIDGE_1_EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BRIDGE_1A_Pin|BRIDGE_3A_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BRIDGE_2_EN_GPIO_Port, BRIDGE_2_EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : LED3_GREEN_Pin LED1_RED_Pin LED4_WHITE_Pin LED2_YELLOW_Pin
                           BRIDGE_1_EN_Pin */
  GPIO_InitStruct.Pin = LED3_GREEN_Pin|LED1_RED_Pin|LED4_WHITE_Pin|LED2_YELLOW_Pin
                          |BRIDGE_1_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BRIDGE_1A_Pin BRIDGE_2_EN_Pin BRIDGE_3A_Pin */
  GPIO_InitStruct.Pin = BRIDGE_1A_Pin|BRIDGE_2_EN_Pin|BRIDGE_3A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM11 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM11) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
