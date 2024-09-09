/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <uxr_transport.h>
#include "uxr/client/transport.h"
#include "uxr/client/client.h"
#include "HelloWorld.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STREAM_HISTORY 4
#define BUFFER_SIZE UXR_CONFIG_CUSTOM_TRANSPORT_MTU *STREAM_HISTORY
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart1;

osThreadId PublishSubscribeTaskHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_RTC_Init(void);
void PublishSubscribeTask(void const *argument);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
  MX_USART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, PublishSubscribeTask, osPriorityNormal, 0, 1024);
  PublishSubscribeTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
   */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 32;
  hrtc.Init.SynchPrediv = 1000;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
   */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */
}
int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
  (void)clock_id;
  RTC_TimeTypeDef rtc_time = {0};
  RTC_DateTypeDef rtc_date = {0};
  struct tm _tm;
  HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
  _tm.tm_year = rtc_date.Year + 70; // RTC_Year rang 0-99,but tm_year since 1900
  _tm.tm_mon = rtc_date.Month;      // RTC_Month rang 1-12,but tm_mon rang 0-11
  _tm.tm_mday = rtc_date.Date;      // RTC_Date rang 1-31 and tm_mday rang 1-31
  _tm.tm_hour = rtc_time.Hours;     // RTC_Hours rang 0-23 and tm_hour rang 0-23
  _tm.tm_min = rtc_time.Minutes;    // RTC_Minutes rang 0-59 and tm_min rang 0-59
  _tm.tm_sec = rtc_time.Seconds;
  tp->tv_sec = mktime(&_tm);
  tp->tv_nsec = (long)((float)(hrtc.Init.SynchPrediv - rtc_time.SubSeconds) / (hrtc.Init.SynchPrediv + 1.0f) * 1000000000.0f);
  return 0;
}
int _write(int file, char *data, int len)
{
  if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
  {
    errno = EBADF;
    return -1;
  }

  // arbitrary timeout 1000
  HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 1000);

  // return # of bytes written - as best we can tell
  return (status == HAL_OK ? len : 0);
}
int fputc(int ch, FILE *f)
{
  // HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
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
  huart1.Init.BaudRate = 115200;
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
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
const char *participant_xml = "<dds>"
                              "<participant>"
                              "<rtps>"
                              "<name>publish_subscribe_participant</name>"
                              "</rtps>"
                              "</participant>"
                              "</dds>";
const char *pub_topic_xml = "<dds>"
                            "<topic>"
                            "<name>rt/PubHelloWorldTopic</name>"
                            "<dataType>pubsub::msg::dds_::HelloWorld_</dataType>"
                            "</topic>"
                            "</dds>";
const char *sub_topic_xml = "<dds>"
                            "<topic>"
                            "<name>rt/SubHelloWorldTopic</name>"
                            "<dataType>pubsub::msg::dds_::HelloWorld_</dataType>"
                            "</topic>"
                            "</dds>";
const char *datawriter_xml = "<dds>"
                             "<data_writer>"
                             "<topic>"
                             "<kind>NO_KEY</kind>"
                             "<name>rt/PubHelloWorldTopic</name>"
                             "<dataType>pubsub::msg::dds_::HelloWorld_</dataType>"
                             "</topic>"
                             "</data_writer>"
                             "</dds>";
const char *datareader_xml = "<dds>"
                             "<data_reader>"
                             "<topic>"
                             "<kind>NO_KEY</kind>"
                             "<name>rt/SubHelloWorldTopic</name>"
                             "<dataType>pubsub::msg::dds_::HelloWorld_</dataType>"
                             "</topic>"
                             "</data_reader>"
                             "</dds>";
uxrStreamId reliable_out;
uxrStreamId reliable_in;
uxrObjectId participant_id;
uxrObjectId pub_topic_id;
uxrObjectId sub_topic_id;
uxrObjectId publisher_id;
uxrObjectId subscriber_id;
uxrObjectId datawriter_id;
uxrObjectId datareader_id;

void on_topic(
    uxrSession *session,
    uxrObjectId object_id,
    uint16_t request_id,
    uxrStreamId stream_id,
    struct ucdrBuffer *ub,
    uint16_t length,
    void *args)
{
  (void)session;

  (void)request_id;
  (void)stream_id;
  (void)length;
  (void)args;

  if (object_id.id == datareader_id.id)
  {
    pubsub_msg_HelloWorld topic;
    pubsub_msg_HelloWorld_deserialize_topic(ub, &topic);

    ucdrBuffer pub_ub;
    uint32_t topic_size = pubsub_msg_HelloWorld_size_of_topic(&topic, 0);
    uxr_prepare_output_stream(session, reliable_out, datawriter_id, &pub_ub, topic_size);
    pubsub_msg_HelloWorld_serialize_topic(&pub_ub, &topic);
  }
}

void PublishSubscribeTask(void const *argument)
{
  uxrCustomTransport transport;

  uxr_set_custom_transport_callbacks(
      &transport,
      true,
      my_custom_transport_open,
      my_custom_transport_close,
      my_custom_transport_write,
      my_custom_transport_read);

  if (!uxr_init_custom_transport(&transport, NULL))
  {
    printf("uxr_init_custom_transport failed.\r\n");
    while (1)
      ;
  }

  uxrSession session;
  uxr_init_session(&session, &transport.comm, 0x08ABCDEF);
  uxr_set_topic_callback(&session, on_topic, 0);
  if (!uxr_create_session(&session))
  {
    printf("uxr_create_session failed.\r\n");
    while (1)
      ;
  }
  // Streams

  reliable_out = uxr_create_output_reliable_stream(&session, output_reliable_stream_buffer, BUFFER_SIZE,
                                                   STREAM_HISTORY);

  reliable_in = uxr_create_input_reliable_stream(&session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

  // Create entities
  participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);

  uint16_t participant_req = uxr_buffer_create_participant_xml(&session, reliable_out, participant_id, 0,
                                                               participant_xml, UXR_REPLACE);

  pub_topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
  sub_topic_id = uxr_object_id(0x02, UXR_TOPIC_ID);

  uint16_t pub_topic_req = uxr_buffer_create_topic_xml(&session, reliable_out, pub_topic_id, participant_id, pub_topic_xml,
                                                       UXR_REPLACE);
  uint16_t sub_topic_req = uxr_buffer_create_topic_xml(&session, reliable_out, sub_topic_id, participant_id, sub_topic_xml,
                                                       UXR_REPLACE);

  publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
  subscriber_id = uxr_object_id(0x01, UXR_SUBSCRIBER_ID);

  const char *publisher_xml = "";
  uint16_t publisher_req = uxr_buffer_create_publisher_xml(&session, reliable_out, publisher_id, participant_id,
                                                           publisher_xml, UXR_REPLACE);
  const char *subscriber_xml = "";
  uint16_t subscriber_req = uxr_buffer_create_subscriber_xml(&session, reliable_out, subscriber_id, participant_id,
                                                             subscriber_xml, UXR_REPLACE);

  datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
  datareader_id = uxr_object_id(0x01, UXR_DATAREADER_ID);

  uint16_t datawriter_req = uxr_buffer_create_datawriter_xml(&session, reliable_out, datawriter_id, publisher_id,
                                                             datawriter_xml, UXR_REPLACE);
  uint16_t datareader_req = uxr_buffer_create_datareader_xml(&session, reliable_out, datareader_id, subscriber_id,
                                                             datareader_xml, UXR_REPLACE);

  // Send create entities message and wait its status
  uint8_t status[7];
  uint16_t requests[7] = {
      participant_req, pub_topic_req, sub_topic_req, publisher_req, subscriber_req, datawriter_req, datareader_req};
  if (!uxr_run_session_until_all_status(&session, 2000, requests, status, 7))
  {
    printf("Error at create entities: participant: %i topic: %i publisher: %i darawriter: %i\n", status[0],
           status[1], status[2], status[3]);
  }

  // Request topics
  uxrDeliveryControl delivery_control = {
      0};
  delivery_control.max_samples = UXR_MAX_SAMPLES_UNLIMITED;
  uxr_buffer_request_data(&session, reliable_out, datareader_id, reliable_in, &delivery_control);

  /* Infinite loop */
  for (;;)
  {
    uxr_run_session_until_data(&session, 500);
  }
  uxr_delete_session(&session);
  uxr_close_custom_transport(&transport);
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
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

#ifdef USE_FULL_ASSERT
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
