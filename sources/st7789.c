#include "stm32f4xx_hal.h"
#include <st7789.h>
#include <stdlib.h>

uint8_t ST7789_Width, ST7789_Height;
sFONT * currentFont;
uint16_t bgColor = BLACK;

static uint8_t txBuf[128];

void ST7789_Init(uint8_t Width, uint8_t Height)
{
  ST7789_Width = Width;
  ST7789_Height = Height;
  
  ST7789_HardReset(); 
  ST7789_SoftReset();
  ST7789_SleepModeExit();

  ST7789_ColorModeSet(ST7789_ColorMode_65K | ST7789_ColorMode_16bit);
  HAL_Delay(10);
  ST7789_MemAccessModeSet(4, 1, 1, 0);
  HAL_Delay(10);
  ST7789_InversionMode(1);
  HAL_Delay(10);
  ST7789_FillScreen(0);
  ST7789_SetBL(10);
  ST7789_DisplayPower(1);
  HAL_Delay(100);
}

void ST7789_HardReset(void)
{
	HAL_GPIO_WritePin(RES_GPIO_Port, RES_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);
  HAL_GPIO_WritePin(RES_GPIO_Port, RES_Pin, GPIO_PIN_SET);
  HAL_Delay(150);
}

void ST7789_SoftReset(void)
{
  ST7789_SendCmd(ST7789_Cmd_SWRESET);
  HAL_Delay(130);
}

void ST7789_SendCmd(uint8_t Cmd)
{
	txBuf[0] = Cmd;
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, txBuf, sizeof(uint8_t), 1);
//	HAL_SPI_Transmit_DMA(&hspi1, &txBuf[0], 1);
}

void ST7789_SendData(uint8_t Data)
{
	txBuf[1] = Data;
	HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET);
	HAL_SPI_Transmit(&hspi1, &txBuf[1], sizeof(uint8_t), 1);
//	HAL_SPI_Transmit_DMA(&hspi1, &txBuf[1], 1);
}

void ST7789_SleepModeEnter( void )
{
	ST7789_SendCmd(ST7789_Cmd_SLPIN);
  HAL_Delay(500);
}

void ST7789_SleepModeExit( void )
{
	ST7789_SendCmd(ST7789_Cmd_SLPOUT);
  HAL_Delay(500);
}


void ST7789_ColorModeSet(uint8_t ColorMode)
{
  ST7789_SendCmd(ST7789_Cmd_COLMOD);
  ST7789_SendData(ColorMode & 0x77);  
}

void ST7789_MemAccessModeSet(uint8_t Rotation, uint8_t VertMirror, uint8_t HorizMirror, uint8_t IsBGR)
{
  uint8_t Value;
  Rotation &= 7; 

  ST7789_SendCmd(ST7789_Cmd_MADCTL);
  
  switch (Rotation)
  {
  case 0:
    Value = 0;
    break;
  case 1:
    Value = ST7789_MADCTL_MX;
    break;
  case 2:
    Value = ST7789_MADCTL_MY;
    break;
  case 3:
    Value = ST7789_MADCTL_MX | ST7789_MADCTL_MY;
    break;
  case 4:
    Value = ST7789_MADCTL_MV;
    break;
  case 5:
    Value = ST7789_MADCTL_MV | ST7789_MADCTL_MX;
    break;
  case 6:
    Value = ST7789_MADCTL_MV | ST7789_MADCTL_MY;
    break;
  case 7:
    Value = ST7789_MADCTL_MV | ST7789_MADCTL_MX | ST7789_MADCTL_MY;
    break;
  }
  
  if (VertMirror)
    Value = ST7789_MADCTL_ML;
  if (HorizMirror)
    Value = ST7789_MADCTL_MH;
  
  if (IsBGR)
    Value |= ST7789_MADCTL_BGR;
  
  ST7789_SendData(Value);
}

void ST7789_InversionMode(uint8_t Mode)
{
  if (Mode)
    ST7789_SendCmd(ST7789_Cmd_INVON);
  else
    ST7789_SendCmd(ST7789_Cmd_INVOFF);
}

void ST7789_FillScreen(uint16_t color)
{
  ST7789_FillRect(0, 0,  ST7789_Width, ST7789_Height, color);
  bgColor = color;
}

void ST7789_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  if ((x >= ST7789_Width) || (y >= ST7789_Height)) return;
  if ((x + w) > ST7789_Width) w = ST7789_Width - x;
  if ((y + h) > ST7789_Height) h = ST7789_Height - y;
  ST7789_SetWindow(x, y, x + w - 1, y + h - 1);
  for (uint32_t i = 0; i < (h * w); i++) ST7789_RamWrite(&color, 1);
}

void ST7789_ClearSector(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
	ST7789_FillRect(x, y, width, height, bgColor);
}

void ST7789_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  ST7789_ColumnSet(x0, x1);
  ST7789_RowSet(y0, y1);
  ST7789_SendCmd(ST7789_Cmd_RAMWR);
}

void ST7789_RamWrite(uint16_t *pBuff, uint16_t Len)
{
  while (Len--) {
    ST7789_SendData(*pBuff >> 8);  
    ST7789_SendData(*pBuff & 0xFF);
  }  
}

void ST7789_ColumnSet(uint16_t ColumnStart, uint16_t ColumnEnd)
{
  if (ColumnStart > ColumnEnd)
    return;
  if (ColumnEnd > ST7789_Width)
    return;
  
  ColumnStart += ST7789_X_Start;
  ColumnEnd += ST7789_X_Start;
  
  ST7789_SendCmd(ST7789_Cmd_CASET);
  ST7789_SendData(ColumnStart >> 8);  
  ST7789_SendData(ColumnStart & 0xFF);  
  ST7789_SendData(ColumnEnd >> 8);  
  ST7789_SendData(ColumnEnd & 0xFF);  
}

void ST7789_RowSet(uint16_t RowStart, uint16_t RowEnd)
{
  if (RowStart > RowEnd)
    return;
  if (RowEnd > ST7789_Height)
    return;
  
  RowStart += ST7789_Y_Start;
  RowEnd += ST7789_Y_Start;
  
  ST7789_SendCmd(ST7789_Cmd_RASET);
  ST7789_SendData(RowStart >> 8);  
  ST7789_SendData(RowStart & 0xFF);  
  ST7789_SendData(RowEnd >> 8);  
  ST7789_SendData(RowEnd & 0xFF);  
}

void ST7789_SetBL(uint8_t Value)
{
  if (Value > 100)
    Value = 100;
  
#if (ST77xx_BLK_PWM_Used)
  //tmr2_PWM_set(ST77xx_PWM_TMR2_Chan, Value);
#else
  if (Value)
;//		HAL_GPIO_WritePin(BLK_GPIO_Port, BLK_Pin, GPIO_PIN_SET);
  else
;//		HAL_GPIO_WritePin(BLK_GPIO_Port, BLK_Pin, GPIO_PIN_RESET);
#endif
}

void ST7789_DisplayPower(uint8_t On)
{
  if (On)
    ST7789_SendCmd(ST7789_Cmd_DISPON);
  else
    ST7789_SendCmd(ST7789_Cmd_DISPOFF);
}

void ST7789_DrawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) 
{
  ST7789_DrawLine(x1, y1, x1, y2, color);
  ST7789_DrawLine(x2, y1, x2, y2, color);
  ST7789_DrawLine(x1, y1, x2, y1, color);
  ST7789_DrawLine(x1, y2, x2, y2, color);
}

void ST7789_DrawRectangleFilled(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t fillcolor) 
{
  if (x1 > x2)
    SwapInt16Values(&x1, &x2);
  if (y1 > y2)
    SwapInt16Values(&y1, &y2);
	ST7789_FillRect(x1, y1, x2 - x1, y2 - y1, fillcolor);
}

void ST7789_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) 
{
  // ������������ �����
  if (x1 == x2)
  {
    // ������������ ����� ������� �������
    if (y1 > y2)
      ST7789_FillRect(x1, y2, 1, y1 - y2 + 1, color);
    else
      ST7789_FillRect(x1, y1, 1, y2 - y1 + 1, color);
    return;
  }
  
  // �������������� �����
  if (y1 == y2)
  {
    // ������������ ����� ������� �������
    if (x1 > x2)
      ST7789_FillRect(x2, y1, x1 - x2 + 1, 1, color);
    else
      ST7789_FillRect(x1, y1, x2 - x1 + 1, 1, color);
    return;
  }
  
  ST7789_DrawLine_Slow(x1, y1, x2, y2, color);
}

void SwapInt16Values(int16_t *pValue1, int16_t *pValue2)
{
  int16_t TempValue = *pValue1;
  *pValue1 = *pValue2;
  *pValue2 = TempValue;
}

void ST7789_DrawLine_Slow(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  const int16_t deltaX = abs(x2 - x1);
  const int16_t deltaY = abs(y2 - y1);
  const int16_t signX = x1 < x2 ? 1 : -1;
  const int16_t signY = y1 < y2 ? 1 : -1;

  int16_t error = deltaX - deltaY;

  ST7789_DrawPixel(x2, y2, color);

  while (x1 != x2 || y1 != y2) 
  {
    ST7789_DrawPixel(x1, y1, color);
    const int16_t error2 = error * 2;
 
    if (error2 > -deltaY) 
    {
      error -= deltaY;
      x1 += signX;
    }
    if (error2 < deltaX)
    {
      error += deltaX;
      y1 += signY;
    }
  }
}

void ST7789_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) ||(x >= ST7789_Width) || (y < 0) || (y >= ST7789_Height))
    return;

  ST7789_SetWindow(x, y, x, y);
  ST7789_RamWrite(&color, 1);
}

void ST7789_DrawCircleFilled(int16_t x0, int16_t y0, int16_t radius, uint16_t fillcolor) 
{
  int x = 0;
  int y = radius;
  int delta = 1 - 2 * radius;
  int error = 0;

  while (y >= 0)
  {
    ST7789_DrawLine(x0 + x, y0 - y, x0 + x, y0 + y, fillcolor);
    ST7789_DrawLine(x0 - x, y0 - y, x0 - x, y0 + y, fillcolor);
    error = 2 * (delta + y) - 1;

    if (delta < 0 && error <= 0) 
    {
      ++x;
      delta += 2 * x + 1;
      continue;
    }
	
    error = 2 * (delta - x) - 1;
		
    if (delta > 0 && error > 0) 
    {
      --y;
      delta += 1 - 2 * y;
      continue;
    }
	
    ++x;
    delta += 2 * (x - y);
    --y;
  }
}

void ST7789_DrawCircle(int16_t x0, int16_t y0, int16_t radius, uint16_t color) 
{
  int x = 0;
  int y = radius;
  int delta = 1 - 2 * radius;
  int error = 0;

  while (y >= 0)
  {
    ST7789_DrawPixel(x0 + x, y0 + y, color);
    ST7789_DrawPixel(x0 + x, y0 - y, color);
    ST7789_DrawPixel(x0 - x, y0 + y, color);
    ST7789_DrawPixel(x0 - x, y0 - y, color);
    error = 2 * (delta + y) - 1;

    if (delta < 0 && error <= 0) 
    {
      ++x;
      delta += 2 * x + 1;
      continue;
    }
	
    error = 2 * (delta - x) - 1;
		
    if (delta > 0 && error > 0) 
    {
      --y;
      delta += 1 - 2 * y;
      continue;
    }
	
    ++x;
    delta += 2 * (x - y);
    --y;
  }
}

void ST7789_DrawChar(uint16_t x, uint16_t y, uint8_t c, uint16_t color)
{
  uint32_t i = 0, j = 0;
  uint8_t *c_t; // The start address of c symbol in the current font table
  uint8_t *pchar; // The current line of the char's block in array
  uint32_t line=0; // The bitwise string
  uint8_t  countOfBytesInLine = ((currentFont->Width + 7) / 8);
  c_t = (uint8_t*) &(currentFont->table[(c-' ') * currentFont->Height * countOfBytesInLine]);

  for(i = 0; i < currentFont->Height; i++)
  {
    pchar = (c_t + countOfBytesInLine * i);
    switch(countOfBytesInLine)
    {
      case 1:
          line =  pchar[0];
          break;
      case 2:
          line =  (pchar[0]<< 8) | pchar[1];
          break;
      case 3:
      default:
        line =  (pchar[0]<< 16) | (pchar[1]<< 8) | pchar[2];
        break;
    }
	uint8_t index = 0;
    for (j = countOfBytesInLine * 8; j > 0; j--)
    {
    	if ((line >> index) & 1u)
    		ST7789_DrawPixel((x + j), y + i, color);
    	else
    		ST7789_DrawPixel((x + j), y + i, bgColor);
    	index++;
    }
  }
}

void ST7789_SetFont(sFONT *pFont)
{
	currentFont = pFont;
}

void ST7789_String(uint16_t x,uint16_t y, char *str, uint16_t color)
{
	  while(*str)
	  {
	    ST7789_DrawChar(x,y,str[0], color);
	    x+=(currentFont->Width);
	    (void)*str++;
	  }
}

void ST7789_DrawPicture(uint8_t x, uint8_t y, const uint16_t * img, uint8_t width, uint8_t height)
{
	for (uint8_t i = 0; i < height; i++) {
		for (uint8_t j = 0; j < width; j++) {
			ST7789_DrawPixel(x + j, y + i, img[i* width + j]);
		}
	}
}
