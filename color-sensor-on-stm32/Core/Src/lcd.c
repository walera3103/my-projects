#include "lcd.h"
#include "main.h"
#include "stm32f7xx_hal.h"

void LCD_Enable_Pulse(void) {
    HAL_GPIO_WritePin(LCD_EN_Port, LCD_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(1); // Opóźnienie 1 ms
    HAL_GPIO_WritePin(LCD_EN_Port, LCD_EN_Pin, GPIO_PIN_RESET);
    HAL_Delay(1); // Opóźnienie 1 ms
}

void LCD_Send_4Bits(uint8_t data) {
    HAL_GPIO_WritePin(LCD_D4_Port, LCD_D4_Pin, (data & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D5_Port, LCD_D5_Pin, (data & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D6_Port, LCD_D6_Pin, (data & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D7_Port, LCD_D7_Pin, (data & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    LCD_Enable_Pulse();
}

void LCD_Send_Command(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_RS_Port, LCD_RS_Pin, GPIO_PIN_RESET); // RS = 0 dla komend
    LCD_Send_4Bits(cmd >> 4); // Wysłanie wyższych 4 bitów
    LCD_Send_4Bits(cmd & 0x0F); // Wysłanie niższych 4 bitów
    HAL_Delay(2); // Opóźnienie dla stabilizacji
}

void LCD_Write_Char(char data) {
    HAL_GPIO_WritePin(LCD_RS_Port, LCD_RS_Pin, GPIO_PIN_SET); // RS = 1 dla danych
    LCD_Send_4Bits(data >> 4); // Wysłanie wyższych 4 bitów
    LCD_Send_4Bits(data & 0x0F); // Wysłanie niższych 4 bitów
    HAL_Delay(2); // Opóźnienie dla stabilizacji
}

void LCD_Init(void) {
    // Ustawienie trybu GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // RS i EN
    GPIO_InitStruct.Pin = LCD_RS_Pin | LCD_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LCD_RS_Port, &GPIO_InitStruct);

    // D4-D7
    GPIO_InitStruct.Pin = LCD_D4_Pin | LCD_D5_Pin | LCD_D6_Pin | LCD_D7_Pin;
    HAL_GPIO_Init(LCD_D4_Port, &GPIO_InitStruct);

    // Inicjalizacja
    HAL_Delay(50);
    LCD_Send_4Bits(0x03); // Tryb 8-bitowy
    HAL_Delay(5);
    LCD_Send_4Bits(0x03);
    HAL_Delay(1);
    LCD_Send_4Bits(0x03);
    LCD_Send_4Bits(0x02); // Przełączenie na tryb 4-bitowy

    // Komendy konfiguracyjne
    LCD_Send_Command(0x28); // Tryb 4-bitowy, 2 linie, 5x8 font
    LCD_Send_Command(0x0C); // Włącz LCD, brak kursora
    LCD_Send_Command(0x06); // Tryb przesuwania kursora w prawo
    LCD_Clear();
}

void LCD_Clear(void) {
    LCD_Send_Command(0x01); // Komenda czyszczenia
    HAL_Delay(2);
}

void LCD_Set_Cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? 0x00 : 0x40; // Adres linii
    address += col;
    LCD_Send_Command(0x80 | address);
}

void LCD_Send_String(char *str) {
    while (*str) {
        LCD_Write_Char(*str++);
    }
}
