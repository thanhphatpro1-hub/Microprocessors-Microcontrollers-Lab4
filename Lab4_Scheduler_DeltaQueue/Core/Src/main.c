/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "scheduler.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIMER_TICK_MS           10UL    /* 1 tick của bộ lập lịch = 10ms */

/* Định nghĩa các chân GPIO cho 5 LED */
#define LED1_GPIO_PORT          GPIOA
#define LED1_PIN                GPIO_PIN_5

#define LED2_GPIO_PORT          GPIOA
#define LED2_PIN                GPIO_PIN_6

#define LED3_GPIO_PORT          GPIOA
#define LED3_PIN                GPIO_PIN_7

#define LED4_GPIO_PORT          GPIOB
#define LED4_PIN                GPIO_PIN_0

#define LED5_GPIO_PORT          GPIOB
#define LED5_PIN                GPIO_PIN_1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/**
 * @brief Biến đếm số tick hệ thống (tăng mỗi 10ms trong ngắt TIM2)
 */
static volatile uint32_t system_ticks = 0UL;

/**
 * @brief Lưu TaskID của task mẫu để kiểm thử hàm SCH_Delete_Task()
 */
static uint32_t g_dummy_task_id = NO_TASK_ID;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
uint32_t get_time(void);
void UART_SendString(const char *str);

/* Khai báo 5 tác vụ điều khiển LED định kỳ */
void Task_LED1(void);
void Task_LED2(void);
void Task_LED3(void);
void Task_LED4(void);
void Task_LED5(void);

/* Khai báo các tác vụ One-shot phục vụ Demonstration */
void Task_Init_Test1(void);
void Task_Init_Test2(void);
void Task_Dummy_Never_Run(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  Chuyển hướng hàm xuất ký tự chuẩn sang USART1 cho printf() trên GCC/STM32CubeIDE.
 * @param  ch: Ký tự cần gửi
 * @retval Ký tự đã gửi
 */
int __io_putchar(int ch) {
    uint8_t c = (uint8_t)ch;
    HAL_UART_Transmit(&huart1, &c, 1U, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief  Hàm hệ thống _write phục vụ thư viện Newlib (printf) trong STM32CubeIDE.
 */
int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    return len;
}

/**
 * @brief  Gửi trực tiếp một chuỗi ký tự qua USART1.
 * @param  str: Con trỏ tới chuỗi ký tự kết thúc bằng '\0'
 */
void UART_SendString(const char *str) {
    if (str != NULL) {
        HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
    }
}

/**
 * @brief  Lấy thời gian hoạt động hiện tại của hệ thống tính bằng mili-giây (ms).
 *         Độ phân giải chính xác theo chu kỳ 10ms của Timer 2.
 * @retval Thời gian hiện tại (ms)
 */
uint32_t get_time(void) {
    return (system_ticks * TIMER_TICK_MS);
}

/* ========================================================================== */
/*                    ĐỊNH NGHĨA 5 TÁC VỤ ĐIỀU KHIỂN LED                      */
/* ========================================================================== */

/**
 * @brief  Task 1: Đảo trạng thái LED 1 (PA5) định kỳ mỗi 0.5s (50 ticks).
 */
void Task_LED1(void) {
    HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
    printf("[Timestamp: %lu ms] LED 1 Toggled (0.5s)\r\n", get_time());
}

/**
 * @brief  Task 2: Đảo trạng thái LED 2 (PA6) định kỳ mỗi 1.0s (100 ticks).
 */
void Task_LED2(void) {
    HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
    printf("[Timestamp: %lu ms] LED 2 Toggled (1.0s)\r\n", get_time());
}

/**
 * @brief  Task 3: Đảo trạng thái LED 3 (PA7) định kỳ mỗi 1.5s (150 ticks).
 */
void Task_LED3(void) {
    HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);
    printf("[Timestamp: %lu ms] LED 3 Toggled (1.5s)\r\n", get_time());
}

/**
 * @brief  Task 4: Đảo trạng thái LED 4 (PB0) định kỳ mỗi 2.0s (200 ticks).
 */
void Task_LED4(void) {
    HAL_GPIO_TogglePin(LED4_GPIO_PORT, LED4_PIN);
    printf("[Timestamp: %lu ms] LED 4 Toggled (2.0s)\r\n", get_time());
}

/**
 * @brief  Task 5: Đảo trạng thái LED 5 (PB1) định kỳ mỗi 2.5s (250 ticks).
 */
void Task_LED5(void) {
    HAL_GPIO_TogglePin(LED5_GPIO_PORT, LED5_PIN);
    printf("[Timestamp: %lu ms] LED 5 Toggled (2.5s)\r\n", get_time());
}

/* ========================================================================== */
/*                ĐỊNH NGHĨA CÁC TÁC VỤ ONE-SHOT DEMONSTRATION                */
/* ========================================================================== */

/**
 * @brief  One-shot Task 1: Chạy 1 lần duy nhất tại thời điểm 3000ms (300 ticks).
 */
void Task_Init_Test1(void) {
    printf("[Timestamp: %lu ms] >>> ONE-SHOT TEST 1 EXPIRED (Expected: 3000 ms) <<<\r\n",
           get_time());
}

/**
 * @brief  One-shot Task 2: Chạy 1 lần duy nhất tại thời điểm 6000ms (600 ticks).
 *         Đồng thời kiểm tra tính năng xóa tác vụ SCH_Delete_Task() trên Delta-Queue.
 */
void Task_Init_Test2(void) {
    printf("[Timestamp: %lu ms] >>> ONE-SHOT TEST 2 EXPIRED (Expected: 6000 ms) <<<\r\n",
           get_time());

    if (g_dummy_task_id != NO_TASK_ID) {
        if (SCH_Delete_Task(g_dummy_task_id) == RETURN_NORMAL) {
            printf("[Timestamp: %lu ms] >>> SCH_Delete_Task(ID=%lu) SUCCESS: Dummy task removed <<<\r\n",
                   get_time(), g_dummy_task_id);
        } else {
            printf("[Timestamp: %lu ms] >>> SCH_Delete_Task(ID=%lu) FAILED <<<\r\n",
                   get_time(), g_dummy_task_id);
        }
        g_dummy_task_id = NO_TASK_ID;
    }
}

/**
 * @brief  Tác vụ mẫu đặt ở 10000ms (1000 ticks), sẽ bị xóa bởi Task_Init_Test2 ở 6000ms.
 */
void Task_Dummy_Never_Run(void) {
    printf("[Timestamp: %lu ms] [ERROR] Dummy task should have been deleted!\r\n",
           get_time());
}

/* ========================================================================== */
/*                        CALLBACK NGẮT ĐỊNH THỜI TIMER 2                     */
/* ========================================================================== */

/**
 * @brief  Hàm callback ngắt tràn Timer (gọi mỗi 10ms bởi TIM2).
 * @param  htim: Con trỏ cấu trúc quản lý Timer
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        system_ticks++;
        SCH_Update();
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

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
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* Tắt bộ đệm stdout để hàm printf() xuất dữ liệu tức thời qua UART1 */
  setvbuf(stdout, NULL, _IONBF, 0);

  /* 1. Khởi tạo bộ lập lịch Delta-Queue */
  SCH_Init();

  UART_SendString("\r\n========================================================\r\n");
  UART_SendString("   STM32F103C8 COOPERATIVE SCHEDULER (DELTA-QUEUE O(1)) \r\n");
  UART_SendString("   Timer 2 Tick = 10 ms | USART1 = 9600 8-N-1           \r\n");
  UART_SendString("========================================================\r\n");

  /*
   * 2. Đăng ký các tác vụ One-shot (Period = 0) trước khi vào vòng lặp chính:
   *    - Task_Init_Test1: chạy 1 lần sau 3000ms (300 ticks)
   *    - Task_Init_Test2: chạy 1 lần sau 6000ms (600 ticks) và xóa Task_Dummy
   *    - Task_Dummy_Never_Run: đặt hẹn tại 10000ms (1000 ticks) để kiểm thử SCH_Delete_Task
   */
  (void)SCH_Add_Task(Task_Init_Test1, 300U, 0U);
  (void)SCH_Add_Task(Task_Init_Test2, 600U, 0U);
  g_dummy_task_id = SCH_Add_Task(Task_Dummy_Never_Run, 1000U, 0U);

  /*
   * 3. Đăng ký 5 tác vụ điều khiển LED định kỳ với Initial Delay lệch pha nhau 10 ticks (100ms)
   *    để tránh xung đột thực thi tại t = 0:
   *    - LED 1 (PA5): Delay =  0 ticks (0ms),   Period =  50 ticks (0.5s)
   *    - LED 2 (PA6): Delay = 10 ticks (100ms), Period = 100 ticks (1.0s)
   *    - LED 3 (PA7): Delay = 20 ticks (200ms), Period = 150 ticks (1.5s)
   *    - LED 4 (PB0): Delay = 30 ticks (300ms), Period = 200 ticks (2.0s)
   *    - LED 5 (PB1): Delay = 40 ticks (400ms), Period = 250 ticks (2.5s)
   */
  (void)SCH_Add_Task(Task_LED1, 0U,  50U);
  (void)SCH_Add_Task(Task_LED2, 10U, 100U);
  (void)SCH_Add_Task(Task_LED3, 20U, 150U);
  (void)SCH_Add_Task(Task_LED4, 30U, 200U);
  (void)SCH_Add_Task(Task_LED5, 40U, 250U);

  printf("[Timestamp: %lu ms] All tasks registered. Starting Timer 2 & entering Super Loop...\r\n\r\n",
         get_time());

  /* 4. Kích hoạt ngắt Timer 2 (chu kỳ 10ms) */
  HAL_TIM_Base_Start_IT(&htim2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    SCH_Dispatch_Tasks();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  *        Cấu hình xung nhịp hệ thống 8 MHz từ bộ dao động nội HSI.
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  *        Tần số bus APB1 = 8 MHz.
  *        Prescaler = 799 -> Tần số đếm = 8.000.000 / (799 + 1) = 10.000 Hz (0.1 ms/xung).
  *        Period (ARR) = 99 -> Chu kỳ tràn = (99 + 1) * 0.1 ms = 10 ms (100 Hz).
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
  htim2.Init.Prescaler = 799;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
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
  * @brief USART1 Initialization Function
  *        Cấu hình UART1: 9600 bps, 8-bit data, No parity, 1 Stop bit (8-N-1).
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
  * @brief GPIO Initialization Function
  *        Cấu hình PA5, PA6, PA7 (LED 1..3) và PB0, PB1 (LED 4..5) ở chế độ Output Push-Pull.
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
  (void)file;
  (void)line;
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
