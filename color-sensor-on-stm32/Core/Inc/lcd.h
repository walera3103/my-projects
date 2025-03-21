#ifndef LCD_H
#define LCD_H

#include "stm32f7xx_hal.h"

/* Definicje pinów LCD */
#define LCD_RS_Pin GPIO_PIN_2
#define LCD_RS_Port GPIOE

#define LCD_EN_Pin GPIO_PIN_4
#define LCD_EN_Port GPIOE

#define LCD_D4_Pin GPIO_PIN_5
#define LCD_D4_Port GPIOE
#define LCD_D5_Pin GPIO_PIN_6
#define LCD_D5_Port GPIOE
#define LCD_D6_Pin GPIO_PIN_3
#define LCD_D6_Port GPIOE
#define LCD_D7_Pin GPIO_PIN_8
#define LCD_D7_Port GPIOF

/* Funkcje LCD */
void LCD_Init(void);
void LCD_Clear(void);
void LCD_Send_String(char *str);
void LCD_Set_Cursor(uint8_t row, uint8_t col);
void LCD_Write_Char(char data);
void LCD_Send_Command(uint8_t cmd);

#endif /* LCD_H */
