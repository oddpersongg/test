#include "stm32f1xx_hal.h"
#include "OLED_Font.h"

#define OLED_ADDR       0x3C        /* SSD1306 I2C 7-bit slave address           */
#define OLED_CTRL_CMD   0x00        /* Control byte: next byte is command      */
#define OLED_CTRL_DATA  0x40        /* Control byte: next byte is data         */

extern I2C_HandleTypeDef hi2c1;     /* from main.c, initialized by MX_I2C1_Init() */

/* ========================== HAL I2C transport ========================= */

/**
  * @brief  Write one command byte to OLED
  * @param  Command : command byte
  * @retval None
  */
void OLED_WriteCommand(uint8_t Command)
{
    uint8_t buf[2];
    buf[0] = OLED_CTRL_CMD;
    buf[1] = Command;
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR << 1, buf, 2, 1000);
}

/**
  * @brief  Write one data byte to OLED
  * @param  Data : data byte
  * @retval None
  */
void OLED_WriteData(uint8_t Data)
{
    uint8_t buf[2];
    buf[0] = OLED_CTRL_DATA;
    buf[1] = Data;
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR << 1, buf, 2, 1000);
}

/* ========================= Display functions ========================== */

/**
  * @brief  OLED set cursor position
  * @param  Y : page 0~7
  * @param  X : column 0~127
  * @retval None
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                    /* set page         */
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));    /* set column high  */
    OLED_WriteCommand(0x00 | (X & 0x0F));            /* set column low   */
}

/**
  * @brief  OLED clear screen
  * @param  None
  * @retval None
  */
void OLED_Clear(void)
{
    uint8_t j, i;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for (i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/**
  * @brief  OLED show one 8x16 character
  * @param  Line   : row 1~4
  * @param  Column : column 1~16
  * @param  Char   : ASCII char
  * @retval None
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
    }
}

/**
  * @brief  OLED show string
  * @param  Line   : start row 1~4
  * @param  Column : start column 1~16
  * @param  String : ASCII string (null-terminated)
  * @retval None
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/**
  * @brief  power function X^Y
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/**
  * @brief  OLED show unsigned decimal number
  * @param  Line   : row 1~4
  * @param  Column : column 1~16
  * @param  Number : 0~4294967295
  * @param  Length : digit count 1~10
  * @retval None
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED show signed decimal number
  * @param  Line   : row 1~4
  * @param  Column : column 1~16
  * @param  Number : -2147483648~2147483647
  * @param  Length : digit count 1~10
  * @retval None
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED show hex number
  * @param  Line   : row 1~4
  * @param  Column : column 1~16
  * @param  Number : 0~0xFFFFFFFF
  * @param  Length : digit count 1~8
  * @retval None
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
        }
    }
}

/**
  * @brief  OLED show binary number
  * @param  Line   : row 1~4
  * @param  Column : column 1~16
  * @param  Number : 0~FFFFFFFF
  * @param  Length : digit count 1~16
  * @retval None
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/**
  * @brief  OLED init sequence
  * @note   I2C1 & GPIO must already be initialized by MX_I2C1_Init() before calling
  * @param  None
  * @retval None
  */
void OLED_Init(void)
{
    HAL_Delay(100);                     /* power-on delay 100ms        */

    /* Check if OLED is on the I2C bus */
    if (HAL_I2C_IsDeviceReady(&hi2c1, OLED_ADDR << 1, 2, 100) != HAL_OK)
    {
        return;                         /* device not ready, skip init  */
    }

    OLED_WriteCommand(0xAE);            /* display off                 */

    OLED_WriteCommand(0xD5);            /* set display clock div       */
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);            /* set multiplex ratio         */
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3);            /* set display offset          */
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40);            /* set display start line      */

    OLED_WriteCommand(0xA1);            /* segment re-map (normal)     */

    OLED_WriteCommand(0xC8);            /* COM scan direction (normal) */

    OLED_WriteCommand(0xDA);            /* COM pin config              */
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);            /* set contrast                */
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);            /* set pre-charge period       */
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);            /* set VCOMH deselect level    */
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);            /* entire display on/off       */

    OLED_WriteCommand(0xA6);            /* normal/inverse display      */

    OLED_WriteCommand(0x8D);            /* charge pump enable          */
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);            /* display on                  */

    OLED_Clear();                       /* clear screen                */
}
