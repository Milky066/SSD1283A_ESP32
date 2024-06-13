#ifndef SSD1283A_H
#define SSD1283A_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//Write Commands

#define LCD_OSCILLATOR 0x00
#define LCD_DRIVER_OUTPUT_CONTROL 0x01
#define LCD_DRIVING_WAVEFORM_CONTROL 0x02
#define LCD_ENTRY_MODE 0x03
#define LCD_COMPARE_REGISTER_1 0x04
#define LCD_COMPARE_REGISTER_2 0x05
#define LCD_DISPLAY_CONTROL 0x07
#define LCD_FRAME_CYCLE_CONTROL 0x0B
#define LCD_POWER_CONTROL_1 0x10
#define LCD_POWER_CONTROL_2 0x11
#define LCD_POWER_CONTROL_3 0x12
#define LCD_POWER_CONTROL_4 0x13
#define LCD_HORIZONTAL_PORCH 0x16
#define LCD_VERTICAL_PORCH 0x17
#define LCD_POWER_CONTROL_5 0x1E
#define LCD_POWER_CONTROL_6 0x1F
#define LCD_RAM_ADDRESS_SET 0x21
#define LCD_WRITE_DATA_TO_GRAM 0x22
#define LCD_RAM_WRITE_DATA_MASK_1 0x23
#define LCD_RAM_WRITE_DATA_MASK_2 0x24
#define LCD_VCOM_OTP_1 0x28
#define LCD_VCOM_OTP_2 0x29
#define LCD_GAMMA_CONTROL_1 0x30
#define LCD_GAMMA_CONTROL_2 0x31
#define LCD_GAMMA_CONTROL_3 0x32
#define LCD_GAMMA_CONTROL_4 0x33
#define LCD_GAMMA_CONTROL_5 0x34
#define LCD_GAMMA_CONTROL_6 0x35
#define LCD_GAMMA_CONTROL_7 0x36
#define LCD_GAMMA_CONTROL_8 0x37
#define LCD_GAMMA_CONTROL_9 0x38
#define LCD_GAMMA_CONTROL_10 0x39
#define LCD_GATE_SCAN_POSITION 0x40
#define LCD_VERTICAL_SCROLL_CONTROL 0x41
#define LCD_1ST_SCREEN_DRIVING_POSITION 0x42
#define LCD_2ND_SCREEN_DRIVING_POSITION 0x43
#define LCD_HORIZONTAL_RAM_ADDRESS_POSITION 0x44
#define LCD_VERTICAL_RAM_ADDRESS_POSITION 0x45
//Extended commands
#define LCD_FURTHER_BIAS_CURRENT_SETTING 0x27
#define LCD_OSCILLATOR_FREQUENCY 0x2C
#define LCD_DELAY 0xFFFF


#define LCD_START_TRANSMIT gpio_set_level(CS_PIN, 0)
#define LCD_STOP_TRANSMIT gpio_set_level(CS_PIN, 1)
#define LCD_CMD_MODE gpio_set_level(DC_PIN, 0)
#define LCD_DATA_MODE gpio_set_level(DC_PIN, 1)

#define CS_ACTIVE gpio_set_level(CS_PIN, 0)
#define CS_IDLE gpio_set_level(CS_PIN, 1)
#define CMD_MODE gpio_set_level(DC_PIN, 0)
#define DATA_MODE gpio_set_level(DC_PIN, 1)




void lcd_initilize(uint8_t cs_pin, uint8_t rst_pin, uint8_t cd_pin, uint8_t led_pin, uint8_t sda_pin, 
uint8_t sck_pin , spi_host_device_t spi_host);
void lcd_reset(void);
void lcd_flood_screen(uint16_t color);
void lcd_set_window(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

#endif