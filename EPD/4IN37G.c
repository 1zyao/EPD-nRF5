/**
 *  @filename   :   epd4in37g.cpp
 *  @brief      :   Implements for e-paper library
 *  @author     :   Waveshare
 *
 *  Copyright (C) Waveshare     2022/08/16
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "EPD_driver.h"
#include "nrf_log.h"

void EPD_4IN37G_Init() 
{
    EPD_Reset(HIGH, 20);

    EPD_WriteCommand(0xAA);
    EPD_WriteByte(0x49);
    EPD_WriteByte(0x55);
    EPD_WriteByte(0x20);
	EPD_WriteByte(0x08);
	EPD_WriteByte(0x09);
    EPD_WriteByte(0x18);

    EPD_WriteCommand(0x01);
    EPD_WriteByte(0x3F);

    EPD_WriteCommand(0x00);
    EPD_WriteByte(0x4F);
    EPD_WriteByte(0x69);

    EPD_WriteCommand(0x05);
    EPD_WriteByte(0x40);
    EPD_WriteByte(0x1F);
    EPD_WriteByte(0x1F);
    EPD_WriteByte(0x2C);

    EPD_WriteCommand(0x08);
    EPD_WriteByte(0x6F);
    EPD_WriteByte(0x1F);
    EPD_WriteByte(0x1F);
    EPD_WriteByte(0x22);

	//===================
	//20211212
	//First setting
    EPD_WriteCommand(0x06);
    EPD_WriteByte(0x6F);
    EPD_WriteByte(0x1F);
    EPD_WriteByte(0x17);
    EPD_WriteByte(0x17);
	//===================
	
    EPD_WriteCommand(0x03);
    EPD_WriteByte(0x00);
    EPD_WriteByte(0x54);
    EPD_WriteByte(0x00);
    EPD_WriteByte(0x44); 

    EPD_WriteCommand(0x50);
    EPD_WriteByte(0x3F);

    EPD_WriteCommand(0x60);
    EPD_WriteByte(0x02);
    EPD_WriteByte(0x00);

	//Please notice that PLL must be set for version 2 IC
    EPD_WriteCommand(0x30);
    EPD_WriteByte(0x08);
	
    EPD_WriteCommand(0x61);
    EPD_WriteByte(0x02);
    EPD_WriteByte(0x00);
    EPD_WriteByte(0x01); 
    EPD_WriteByte(0x70); 

    EPD_WriteCommand(0xE3);
    EPD_WriteByte(0x2F);

    EPD_WriteCommand(0x84);
    EPD_WriteByte(0x01);
}

/**
 *  @brief: Wait until the busy_pin goes LOW
 */
void ReadBusyH(void) {
    EPD_WaitBusy(LOW, 30000);
}

void EPD_4IN37G_WaitBusy(uint16_t timeout) {
    EPD_WaitBusy(LOW, timeout);
}


static void EPD_4IN37G_PowerOn(void)
{
    EPD_WriteCommand(0x04);
    EPD_4IN37G_WaitBusy(100);
}

static void EPD_4IN37G_PowerOff(void)
{
    EPD_WriteCommand(0x02);
    EPD_WriteByte(0x00);
    EPD_4IN37G_WaitBusy(100);
}

// Read temperature from driver chip
int8_t EPD_4IN37G_Read_Temp(void)
{
    EPD_WriteCommand(0x40);
    EPD_4IN37G_WaitBusy(100);
    return (int8_t) EPD_ReadByte();
}

// Force temperature (will trigger OTP LUT switch)
void EPD_4IN37G_Force_Temp(int8_t value)
{
    EPD_WriteCommand(0xE0);
    EPD_WriteByte(0x02);
    EPD_WriteCommand(0xE5);
    EPD_WriteByte(value);
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
void EPD_4IN37G_Refresh(void)
{
    NRF_LOG_DEBUG("[EPD]: refresh begin\n");
    EPD_4IN37G_PowerOn();
    NRF_LOG_DEBUG("[EPD]: temperature: %d\n", EPD_4IN37G_Read_Temp());
    EPD_WriteCommand(0x12);
    EPD_WriteByte(0x00);
    delay(100);
    EPD_4IN37G_WaitBusy(30000);
    EPD_4IN37G_PowerOff();
    NRF_LOG_DEBUG("[EPD]: refresh end\n");
}


static void EPD_4IN37G_Write_RAM(uint8_t cmd, uint8_t value)
{
    epd_model_t *EPD = epd_get();
    uint16_t Width = (EPD->width + 3) / 4;
    uint16_t Height = EPD->height;

    EPD_WriteCommand(cmd);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_WriteByte((value<<6) | (value<<4) | (value<<2) | value);
        }
    }
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_4IN37G_Clear()
{
    EPD_4IN37G_PowerOn();
    EPD_4IN37G_Write_RAM(0x10, 0x01);
    EPD_4IN37G_Refresh();
    EPD_4IN37G_PowerOn();
    EPD_4IN37G_Write_RAM(0x10, 0x00);
    EPD_4IN37G_Refresh();
    EPD_4IN37G_PowerOn();
    EPD_4IN37G_Write_RAM(0x10, 0x02);
    EPD_4IN37G_Refresh();
    EPD_4IN37G_PowerOn();
    EPD_4IN37G_Write_RAM(0x10, 0x03);
    EPD_4IN37G_Refresh();
    EPD_4IN37G_PowerOn();
    EPD_4IN37G_Write_RAM(0x10, 0x01);
    EPD_4IN37G_Refresh();
}

static void _setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    EPD_WriteCommand(0x83); // partial window
    EPD_WriteByte(x / 256);
    EPD_WriteByte(x % 256);
    EPD_WriteByte((x + w - 1) / 256);
    EPD_WriteByte((x + w - 1) % 256);
    EPD_WriteByte(y / 256);
    EPD_WriteByte(y % 256);
    EPD_WriteByte((y + h - 1) / 256);
    EPD_WriteByte((y + h - 1) % 256);
    EPD_WriteByte(0x01);
}
/*
void EPD_4IN37G_Write_Image(uint8_t *black, uint8_t *color, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    epd_model_t *EPD = epd_get();
    int16_t wb = (w + 3) / 4; // width bytes, bitmaps are padded
    x -= x % 4; // byte boundary
    w = wb * 4; // byte boundary
    int16_t x1 = x < 0 ? 0 : x; // limit
    int16_t y1 = y < 0 ? 0 : y; // limit
    int16_t w1 = x + w < EPD->width ? w : EPD->width - x; // limit
    int16_t h1 = y + h < EPD->height ? h : EPD->height - y; // limit
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
	NRF_LOG_DEBUG("Update area: x=%d, y=%d, w=%d, h=%d\n", x1, y1, w1, h1);
    if ((w1 <= 0) || (h1 <= 0)) return;
    _setPartialRamArea(x, y, w, h);
    EPD_WriteCommand(0x10);
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 4; j++)
      {
        uint8_t data;
        // use wb, h of bitmap for index!
        uint32_t idx = j + dx / 4 + (i + dy) * wb;
        data = black[idx];
        EPD_WriteByte(data);
      }
    }
}*/

void EPD_4IN37G_Write_Image(uint8_t* data1, uint8_t* data2, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    epd_model_t *EPD = epd_get();
    
    uint16_t wb = (w + 3) / 4; // width bytes, bitmaps are padded (4 pixels per byte)
    x -= x % 4; // byte boundary (4 pixels)
    w = wb * 4; // byte boundary
    
    // Limit checking
    uint16_t x1 = x < 0 ? 0 : x;
    uint16_t y1 = y < 0 ? 0 : y;
    uint16_t w1 = x + w < EPD->width ? w : EPD->width - x;
    uint16_t h1 = y + h < EPD->height ? h : EPD->height - y;
    
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx;
    h1 -= dy;
    
    if ((w1 <= 0) || (h1 <= 0)) return;
    
        _setPartialRamArea(x1, y1, w1, h1);
        EPD_WriteCommand(0x10); // data start command
        
        for (uint16_t i = 0; i < h1; i++)
        {
            for (uint16_t j = 0; j < w1 / 4; j++)
            {
                uint8_t data;
                uint32_t idx = j + dx / 4 + (i + dy) * wb;
                
                    data = data1[idx];
                
                EPD_WriteByte(data);
            }
        }
}
/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_4IN37G_Sleep(void)
{
    EPD_4IN37G_PowerOff();
	
    EPD_WriteCommand(0x07); // DEEP_SLEEP
    EPD_WriteByte(0XA5);
}

// Declare driver and models
static epd_driver_t epd_drv_4in37g = {
    .init = EPD_4IN37G_Init,
    .clear = EPD_4IN37G_Clear,
    .write_image = EPD_4IN37G_Write_Image,
    .refresh = EPD_4IN37G_Refresh,
    .sleep = EPD_4IN37G_Sleep,
    .read_temp = EPD_4IN37G_Read_Temp,
    .force_temp = EPD_4IN37G_Force_Temp,
    .cmd_write_ram1 = 0x10,
    .cmd_write_ram2 = 0x10,
};


const epd_model_t epd_4in37g_437_bwry = {
    .id = EPD_4IN37G_437_BWRY,
    .drv = &epd_drv_4in37g,
    .width = 512,
    .height = 368,
    .bwr = true,
};

/* END OF FILE */


