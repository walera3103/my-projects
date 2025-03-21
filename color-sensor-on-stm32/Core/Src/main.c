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
#include "eth.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  // For boolean data type (bool, true, false)
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define INPUT_UART_BUFFER_SIZE 4
#define ISL29125_I2C_ADDRESS (0x44 << 1)  // Adres 7-bitowy przesunięty w lewo
# define PWM_FREQUENCY 1000 // Czę stotliwo ść PWM 1 kHz
# define FRAME_LENGTH 5
# define FRAME_SIZE 12 // Komenda ma dł ugość 9 znak ów: R050G025B075
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char uart_buffer[50];
char input_uart_buffer[INPUT_UART_BUFFER_SIZE];
char temp_buffer[10];
volatile uint8_t input_uart_index = 0;
volatile uint8_t input_data_ready = 0;
const int MAX_RED_PWM = 21;
const int MAX_GREEN_PWM = 35;
const int MAX_BLUE_PWM = 20;
int R_value = 0;
int G_value = 0;
int B_value = 0;
int R_PWM_channel = 0;
int G_PWM_channel = 0;
int B_PWM_channel = 0;
int color_switch = 0;
int pause_for_user_input = 200;
int pause_for_led = 0;
int max_PWM_red_value = MAX_RED_PWM;
int max_PWM_green_value = MAX_GREEN_PWM;
int max_PWM_blue_value = MAX_BLUE_PWM;
int test_sizeof = 0;
bool normal_mode = true;
bool test;
char previous_temp_buffer[10];
char buffor_for_lcd[10];
char mode_normal[] = "mode:normal";
char mode_high[] = "mode:high";
char help[] = "help";

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//    if (huart->Instance == USART3) {
//        HAL_UART_Transmit(&huart3, (uint8_t *)"Interrupt triggered\r\n", strlen("Interrupt triggered\r\n"), HAL_MAX_DELAY);
//        //R_value = 0;
//        // Zwiększ indeks bufora i odbierz kolejny bajt
//        if (input_uart_index < INPUT_UART_BUFFER_SIZE - 1) {
//            input_uart_index++;
//            HAL_UART_Receive_IT(&huart3, (uint8_t *)&input_uart_buffer[input_uart_index], 1);
//        } else {
//            input_data_ready = 1; // Ustaw flagę gotowości danych
//        }
//    } else {
//    	exit(0);
//    }
//}

void ISL29125_Init(void) {
    uint8_t config_data[2];

    // Ustawienie trybu pracy
    config_data[0] = 0x01;  // Rejestr konfiguracji
    config_data[1] = 0x05;  // RGB mode, high range, 10-bit resolution
    if (HAL_I2C_Master_Transmit(&hi2c1, ISL29125_I2C_ADDRESS, config_data, 2, HAL_MAX_DELAY) != HAL_OK) {
        HAL_UART_Transmit(&huart3, (uint8_t *)"I2C error in step 1\r\n", strlen("I2C error in step 1\r\n"), HAL_MAX_DELAY);
        Error_Handler();
    }

    // Ustawienie progu pomiarowego (opcjonalne)
    config_data[0] = 0x02;  // Rejestr parametru
    config_data[1] = 0x00;  // Domyślny próg
    if (HAL_I2C_Master_Transmit(&hi2c1, ISL29125_I2C_ADDRESS, config_data, 2, HAL_MAX_DELAY) != HAL_OK) {
        HAL_UART_Transmit(&huart3, (uint8_t *)"I2C error in step 2\r\n", strlen("I2C error in step 2\r\n"), HAL_MAX_DELAY);
        Error_Handler();
    }
}

uint16_t ISL29125_ReadColor(uint8_t color_register) {
    uint8_t reg = color_register;
    uint8_t data[2];

    // Wyślij adres rejestru do odczytu
    if (HAL_I2C_Master_Transmit(&hi2c1, ISL29125_I2C_ADDRESS, &reg, 1, HAL_MAX_DELAY) != HAL_OK) {
        HAL_UART_Transmit(&huart3, (uint8_t *)"I2C error during read transmit\r\n", strlen("I2C error during read transmit\r\n"), HAL_MAX_DELAY);
        Error_Handler();
    }

    // Odczytaj dane (2 bajty)
    if (HAL_I2C_Master_Receive(&hi2c1, ISL29125_I2C_ADDRESS, data, 2, HAL_MAX_DELAY) != HAL_OK) {
        HAL_UART_Transmit(&huart3, (uint8_t *)"I2C error during read receive\r\n", strlen("I2C error during read receive\r\n"), HAL_MAX_DELAY);
        Error_Handler();
    }

    // Połącz 2 bajty w 16-bitową wartość
    return (data[1] << 8) | data[0];
}

uint8_t map_value(uint16_t input_value, uint16_t in_min, uint16_t in_max, uint8_t out_min, uint8_t out_max) {
    return (uint8_t)(((input_value - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min);
}

void PWM_Config ( TIM_HandleTypeDef *htim, uint32_t channel, uint32_t frequency, uint32_t duty_cycle ) {
	/* USER CODE BEGIN PWM_Config */
	uint32_t timer_clock = HAL_RCC_GetPCLK1Freq () ; // Zakładamy , że TIM2 jest podłączony do APB1
	uint32_t prescaler = ( timer_clock / ( frequency * 1000) ) - 1; // Preskaler ,aby uzyska ć 1 kHz
	uint32_t period = 1000 - 1; // Okres dla PWM , aby uzyska ć 1000 krok ów ( maks. wypełnienie = 100%)
	uint32_t pulse = ( duty_cycle * ( period + 1) ) / 100; // Wyliczenie wypełnienia na podstawie %

	// Ustawienia preskalera i okresu
	htim -> Init.Prescaler = prescaler;
	htim -> Init.Period = period;
	HAL_TIM_PWM_Init(htim);

	// Konfiguracja wype ł nienia dla wybranego kana łu
	TIM_OC_InitTypeDef sConfigOC;
	sConfigOC . OCMode = TIM_OCMODE_PWM1;
	sConfigOC . Pulse = pulse;
	sConfigOC . OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC . OCFastMode = TIM_OCFAST_DISABLE;

	// Konfiguracja wyjś cia PWM dla wybranego kana łu
	HAL_TIM_PWM_ConfigChannel (htim, &sConfigOC , channel);

/* USER CODE END PWM_Config */
}

void high_mode_Config() {
	normal_mode = false;
	max_PWM_red_value = 100;
	max_PWM_green_value = 100;
	max_PWM_blue_value = 100;

	LCD_Set_Cursor(0, 0);
	LCD_Send_String("MODE: high  ");
}

void normal_mode_Config() {
	normal_mode = true;
	max_PWM_red_value = MAX_RED_PWM;
	max_PWM_green_value = MAX_GREEN_PWM;
	max_PWM_blue_value = MAX_BLUE_PWM;
	if(R_PWM_channel > max_PWM_red_value) {
		R_PWM_channel = max_PWM_red_value;
	}
	if(G_PWM_channel > max_PWM_green_value) {
		G_PWM_channel = max_PWM_green_value;
	}
	if(B_PWM_channel > max_PWM_blue_value) {
		B_PWM_channel = max_PWM_blue_value;
	}

	LCD_Set_Cursor(0, 0);
	LCD_Send_String("MODE: normal");
}

void rewrite_previous_temp_buffer() {
	for(int k = 0; k < 4; k++) {
		previous_temp_buffer[k] = temp_buffer[k];
	}
}

int check_mode_normal() {
	for(int i = 0; i < 11; i++) {
		if(temp_buffer[i] != mode_normal[i]) {
			return 0;
			exit(0);
		}
	}
	return 1;
}

int check_mode_high() {
	for(int i = 0; i < 9; i++) {
		if(temp_buffer[i] != mode_high[i]) {
			return 0;
			exit(0);
		}
	}
	return 1;
}

int check_help() {
	for(int i = 0; i < 4; i++) {
		if(temp_buffer[i] != help[i]) {
			return 0;
			exit(0);
		}
	}
	return 1;
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
  MX_ETH_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Transmit(&huart3, (uint8_t *)"Starting I2C scan...\r\n", strlen("Starting I2C scan...\r\n"), HAL_MAX_DELAY);
  I2C_Scan();
  LCD_Init();

  snprintf(uart_buffer, sizeof(uart_buffer), "Hello world\r\n");
  HAL_UART_Transmit(&huart3, (uint8_t *)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);

  ISL29125_Init();
  snprintf(uart_buffer, sizeof(uart_buffer), "ISL29125 Initialized\r\n");
  HAL_UART_Transmit(&huart3, (uint8_t *)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);

  LCD_Clear();
  LCD_Set_Cursor(0, 0);
  LCD_Send_String("MODE: normal");
  LCD_Set_Cursor(0, 13);
  LCD_Send_String("C:R");
  LCD_Set_Cursor(1, 0);
  LCD_Send_String("R0");
  LCD_Set_Cursor(1, 5);
  LCD_Send_String("G0");
  LCD_Set_Cursor(1, 10);
  LCD_Send_String("B0");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  uint16_t green_raw = ISL29125_ReadColor(0x09);    // Odczyt RED
	  uint16_t red_raw = ISL29125_ReadColor(0x0B);  // Odczyt GREEN
	  uint16_t blue_raw = ISL29125_ReadColor(0x0D);   // Odczyt BLUE

	  // Mapowanie wartości do zakresu 0–255
	  uint8_t red = map_value(red_raw, 0, 65535, 0, 255);
	  uint8_t green = map_value(green_raw, 0, 65535, 0, 255);
	  uint8_t blue = map_value(blue_raw, 0, 65535, 0, 255);

	  red = red * 1;
	  green = green; // *6
	  blue = blue * 1;

	  test = HAL_GPIO_ReadPin(MODE_GPIO_Port, MODE_Pin);

	  test_sizeof = sizeof(temp_buffer);

	  if(HAL_GPIO_ReadPin(MODE_GPIO_Port, MODE_Pin) == 1) {
		  if(normal_mode == true) {
			  high_mode_Config();
		  } else if (normal_mode == false) {
			  normal_mode_Config();
		  }
	  }

	  if(HAL_GPIO_ReadPin(COLOR_SWITCH_GPIO_Port, COLOR_SWITCH_Pin) == 1) {
		  if(color_switch == 0) {
			  color_switch = 1;
			  LCD_Set_Cursor(0, 15);
			  LCD_Send_String("G");

		  } else if(color_switch == 1) {
			  color_switch = 2;
			  LCD_Set_Cursor(0, 15);
			  LCD_Send_String("B");
		  } else if(color_switch == 2) {
			  color_switch = 0;
		      LCD_Set_Cursor(0, 15);
			  LCD_Send_String("R");
		  }
	  }

	  if(HAL_GPIO_ReadPin(ADD_COLOR_GPIO_Port, ADD_COLOR_Pin) == 1) {
		  if(color_switch == 0 && R_value < 255) {
			  R_value++;
			  sprintf(buffor_for_lcd, "%d", R_value);
			  LCD_Set_Cursor(1, 1);
			  LCD_Send_String(buffor_for_lcd);
			  if(R_value < 10) {
				  LCD_Set_Cursor(1, 2);
				  LCD_Send_String(" ");
			  }
			  if(R_value < 100) {
				  LCD_Set_Cursor(1, 3);
				  LCD_Send_String(" ");
			  }
		  } else if (color_switch == 1 && G_value < 255) {
			  G_value++;
			  sprintf(buffor_for_lcd, "%d", G_value);
			  LCD_Set_Cursor(1, 6);
			  LCD_Send_String(buffor_for_lcd);
			  if(G_value < 10) {
				  LCD_Set_Cursor(1, 7);
				  LCD_Send_String(" ");
			  }
			  if(G_value < 100) {
				  LCD_Set_Cursor(1, 8);
				  LCD_Send_String(" ");
			  }
		  } else if (color_switch == 2 && B_value < 255) {
			  B_value++;
			  sprintf(buffor_for_lcd, "%d", B_value);
			  LCD_Set_Cursor(1, 11);
			  LCD_Send_String(buffor_for_lcd);
			  if(B_value < 10) {
				  LCD_Set_Cursor(1, 12);
				  LCD_Send_String(" ");
			  }
			  if(B_value < 100) {
				  LCD_Set_Cursor(1, 13);
				  LCD_Send_String(" ");
			  }
		  }
	  }

	  if(HAL_GPIO_ReadPin(SUBTRACT_COLOR_GPIO_Port, SUBTRACT_COLOR_Pin) == 1) {
		  if(color_switch == 0 && R_value > 0) {
			  R_value--;
			  sprintf(buffor_for_lcd, "%d", R_value);
			  LCD_Set_Cursor(1, 1);
			  LCD_Send_String(buffor_for_lcd);
			  if(R_value < 10) {
				  LCD_Set_Cursor(1, 2);
				  LCD_Send_String(" ");
			  }
			  if(R_value < 100) {
				  LCD_Set_Cursor(1, 3);
				  LCD_Send_String(" ");
			  }
		  } else if(color_switch == 1 && G_value > 0) {
			  G_value--;
			  sprintf(buffor_for_lcd, "%d", G_value);
			  LCD_Set_Cursor(1, 6);
			  LCD_Send_String(buffor_for_lcd);
			  if(G_value < 10) {
				  LCD_Set_Cursor(1, 7);
				  LCD_Send_String(" ");
			  }
			  if(G_value < 100) {
				  LCD_Set_Cursor(1, 8);
				  LCD_Send_String(" ");
			  }

		  } else if(color_switch == 2 && B_value > 0) {
			  B_value--;
			  sprintf(buffor_for_lcd, "%d", B_value);
			  LCD_Set_Cursor(1, 11);
			  LCD_Send_String(buffor_for_lcd);
			  if(B_value < 10) {
				  LCD_Set_Cursor(1, 12);
				  LCD_Send_String(" ");
			  }
			  if(B_value < 100) {
				  LCD_Set_Cursor(1, 13);
				  LCD_Send_String(" ");
			  }
		  }
	  }

	  if(pause_for_led >= 500) {
		  if(red < R_value) {
			  if(R_PWM_channel < max_PWM_red_value) {
				  R_PWM_channel++;
			  }
		  } else if(red > R_value) {
			  if(R_PWM_channel > 0) {
				  R_PWM_channel--;
			  }
		  }

		  if(green < G_value) {
			  if(G_PWM_channel < max_PWM_green_value) {
				  G_PWM_channel++;
			  }
		  } else if(green > G_value) {
			  if(G_PWM_channel > 0) {
				  G_PWM_channel--;
			  }
		  }

		  if(blue < B_value) {
			  if(B_PWM_channel < max_PWM_blue_value) {
				  B_PWM_channel++;
			  }
		  } else if(blue > B_value) {
			  if(B_PWM_channel > 0) {
				  B_PWM_channel--;
			  }
		  }
		  pause_for_led = 0;
	  } else {
		  pause_for_led = pause_for_led + pause_for_user_input;
	  }

	  PWM_Config (&htim3 , TIM_CHANNEL_1 , PWM_FREQUENCY , R_PWM_channel ) ;
	  PWM_Config (&htim3 , TIM_CHANNEL_2 , PWM_FREQUENCY , G_PWM_channel ) ;
	  PWM_Config (&htim3 , TIM_CHANNEL_3 , PWM_FREQUENCY , B_PWM_channel ) ;

	  HAL_TIM_PWM_Start (&htim3 , TIM_CHANNEL_1 ) ;
	  HAL_TIM_PWM_Start (&htim3 , TIM_CHANNEL_2 ) ;
	  HAL_TIM_PWM_Start (&htim3 , TIM_CHANNEL_3 ) ;

	  snprintf(uart_buffer, sizeof(uart_buffer), "R: %u, G: %u, B: %u\r\n", red, green, blue);
	  HAL_UART_Transmit(&huart3, (uint8_t *)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);


	  HAL_UART_Receive(&huart3, (uint8_t *)temp_buffer, 11, pause_for_user_input);
	  // Wyślij odebrane dane z powrotem
	  HAL_UART_Transmit(&huart3, (uint8_t *)"Received: ", strlen("Received: "), HAL_MAX_DELAY);

	  HAL_UART_Transmit(&huart3, (uint8_t *)temp_buffer, 11, HAL_MAX_DELAY);
	  HAL_UART_Transmit(&huart3, (uint8_t *)"\r\n", 2, HAL_MAX_DELAY);

	  if(normal_mode == 1) {
		  HAL_UART_Transmit(&huart3, (uint8_t *)"MODE: normal\r\n", strlen("MODE: normal\r\n"), HAL_MAX_DELAY);
	  }
	  else if(normal_mode == 0) {
		  HAL_UART_Transmit(&huart3, (uint8_t *)"MODE: high\r\n", strlen("MODE: high\r\n"), HAL_MAX_DELAY);
	  }

	  for(int i = 0; i < 11; i++) {
		  if (previous_temp_buffer[i] != temp_buffer[i]) {
			  if(temp_buffer[11] != '\000' || temp_buffer[0] == '\000' || temp_buffer[1] == '\000' || temp_buffer[2] == '\000' || temp_buffer[3] == '\000') {
				  HAL_UART_Transmit(&huart3, (uint8_t *)"incorrect commnand\r\n", strlen("incorrect commnand\r\n"), HAL_MAX_DELAY);
				  break;
			  }
			  if(check_mode_normal() == 1) {
				  normal_mode_Config();
				  rewrite_previous_temp_buffer();
				  break;
			  }
			  else if(check_mode_high() == 1) {
				  high_mode_Config();
				  temp_buffer[9] = '\000';
				  temp_buffer[10] = '\000';
				  rewrite_previous_temp_buffer();
				  break;
			  }
			  else if(check_help() == 1) {
				  HAL_UART_Transmit(&huart3, (uint8_t *)"mode:normal - enter normal mode\r\n", strlen("mode:normal - enter normal mode\r\n"), HAL_MAX_DELAY);
				  HAL_UART_Transmit(&huart3, (uint8_t *)"mode:high - enter high mode\r\n", strlen("mode:high - enter high mode\r\n"), HAL_MAX_DELAY);
				  HAL_UART_Transmit(&huart3, (uint8_t *)"COLORXXX - enter parameter to special color, where:\r\n", strlen("COLORXXX - enter parameter to special color, where:\r\n"), HAL_MAX_DELAY);
				  HAL_UART_Transmit(&huart3, (uint8_t *)"in the place COLOR - enter your desired color (R, G, B)\r\n", strlen("in the place COLOR - enter your desired color (R, G, B)\r\n"), HAL_MAX_DELAY);
				  //HAL_UART_Transmit(&huart3, (uint8_t *)"color (R, G, B)\r\n", strlen("color (R, G, B)\r\n"), HAL_MAX_DELAY);
				  HAL_UART_Transmit(&huart3, (uint8_t *)"in the place XXX - enter your desired color value (0 - 255)\r\n", strlen("in the place XXX - enter your desired color value (0 - 255)\r\n"), HAL_MAX_DELAY);
				  HAL_UART_Transmit(&huart3, (uint8_t *)"for example: R175\r\n", strlen("for example: R175\r\n"), HAL_MAX_DELAY);
				  //HAL_UART_Transmit(&huart3, (uint8_t *)"R175\r\n", strlen("R175\r\n"), HAL_MAX_DELAY);
				  for(int k = 4; k < 11; k++) {
					  temp_buffer[k] = '\000';
				  }
				  rewrite_previous_temp_buffer();
				  break;
			  }
			  else if(((temp_buffer[1] - '0') * 100 + (temp_buffer[2] - '0') * 10 + temp_buffer[3] - '0') > 255 || ((temp_buffer[1] - '0') * 100 + (temp_buffer[2] - '0') * 10 + temp_buffer[3] - '0') < 0) {
				  HAL_UART_Transmit(&huart3, (uint8_t *)"incorrect commnand\r\n", strlen("incorrect commnand\r\n"), HAL_MAX_DELAY);
				  break;
			  }
			  if (temp_buffer[0] == 'R') {
				  R_value = (temp_buffer[1] - '0') * 100 + (temp_buffer[2] - '0') * 10 + temp_buffer[3] - '0';
				  sprintf(buffor_for_lcd, "%d", R_value);
				  LCD_Set_Cursor(1, 1);
				  LCD_Send_String(buffor_for_lcd);
				  if(R_value < 10) {
					  LCD_Set_Cursor(1, 2);
					  LCD_Send_String(" ");
				  }
				  if(R_value < 100) {
					  LCD_Set_Cursor(1, 3);
					  LCD_Send_String(" ");
				  }
			  } else if (temp_buffer[0] == 'G') {
				  G_value = (temp_buffer[1] - '0') * 100 + (temp_buffer[2] - '0') * 10 + temp_buffer[3] - '0';
				  sprintf(buffor_for_lcd, "%d", G_value);
				  LCD_Set_Cursor(1, 6);
				  LCD_Send_String(buffor_for_lcd);
				  if(G_value < 10) {
					  LCD_Set_Cursor(1, 7);
					  LCD_Send_String(" ");
				  }
				  if(G_value < 100) {
					  LCD_Set_Cursor(1, 8);
					  LCD_Send_String(" ");
				  }
			  } else if (temp_buffer[0] == 'B'){
				  B_value = (temp_buffer[1] - '0') * 100 + (temp_buffer[2] - '0') * 10 + temp_buffer[3] - '0';
				  sprintf(buffor_for_lcd, "%d", B_value);
				  LCD_Set_Cursor(1, 11);
				  LCD_Send_String(buffor_for_lcd);
				  if(B_value < 10) {
					  LCD_Set_Cursor(1, 12);
					  LCD_Send_String(" ");
				  }
				  if(B_value < 100) {
					  LCD_Set_Cursor(1, 13);
					  LCD_Send_String(" ");
				  }
			  } else {
				  HAL_UART_Transmit(&huart3, (uint8_t *)"incorrect commnand\r\n", strlen("incorrect commnand\r\n"), HAL_MAX_DELAY);
				  break;
			  }
			  for(int k = 4; k < 11; k++) {
				  temp_buffer[k] = '\000';
			  }
			  rewrite_previous_temp_buffer();
			  break;
		  }
	  }

	  //HAL_Delay(500);  // Opóźnienie 500 ms
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void I2C_Scan() {
    char msg[64];
    HAL_UART_Transmit(&huart3, (uint8_t *)"Scanning I2C bus...\r\n", strlen("Scanning I2C bus...\r\n"), HAL_MAX_DELAY);

    for (uint8_t address = 1; address < 128; address++) {
        // Sprawdź, czy urządzenie odpowiada na dany adres
        if (HAL_I2C_IsDeviceReady(&hi2c1, (address << 1), 1, 10) == HAL_OK) {
            snprintf(msg, sizeof(msg), "Device found at address: 0x%02X\r\n", address);
            HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        }
    }

    HAL_UART_Transmit(&huart3, (uint8_t *)"I2C scan complete.\r\n", strlen("I2C scan complete.\r\n"), HAL_MAX_DELAY);
}

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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line); */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
