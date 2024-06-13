#include "ssd1283a.h"

static uint8_t _cs, _rst, _cd, _led, _sda, _sck;
static uint8_t _x_start, _x_end, _y_start, _y_end;
static spi_device_handle_t _spi_device_handle;
static spi_host_device_t _spi_host = -1;
static const char* TAG = "SSD1283A";

static void lcd_cs_active(spi_transaction_t *t);
static void lcd_cs_idle(spi_transaction_t *t);
static void write_cmd_data(uint16_t cmd, uint16_t data);


#define write8(data) spi_device_transmit(_spi_device_handle, &(spi_transaction_t) { \
    .length = 8, \
    .tx_data = { data }, \
    .flags = SPI_TRANS_USE_TXDATA \
})
#define write16(data) spi_device_transmit(_spi_device_handle, &(spi_transaction_t) { \
    .length = 16, \
    .tx_data = { (uint8_t )(data >> 8), (uint8_t )data }, \
    .flags = SPI_TRANS_USE_TXDATA \
})
#define CS_PIN    5                 // Chip Select pin
#define RST_PIN   33                // Reset pin
#define DC_PIN    27                // A0 Pin
#define SDA_PIN   23                // SPI MOSI pin
#define SCK_PIN   18                // SPI Clock
#define LED_PIN   32
#define write_cmd_8(x) CMD_MODE; write8(x)
#define write_data_8(x)  DATA_MODE; write8(x)
#define writeCmd16(x)  CMD_MODE; write16(x)
#define writeData16(x)  DATA_MODE; write16(x)
#define writeCmdData8(a, d) CMD_MODE; write8(a); DATA_MODE; write8(d)
#define writeCmdData16(a, d)  CMD_MODE; write8(a>>8); write8(a); DATA_MODE; write8(d>>8); write8(d) 
#define CS_ACTIVE gpio_set_level(CS_PIN, 0)
#define CS_IDLE gpio_set_level(CS_PIN, 1)
#define CMD_MODE gpio_set_level(DC_PIN, 0)
#define DATA_MODE gpio_set_level(DC_PIN, 1)

void lcd_initilize(uint8_t cs_pin, uint8_t rst_pin, uint8_t cd_pin, uint8_t led_pin, 
uint8_t sda_pin, uint8_t sck_pin , spi_host_device_t spi_host){

    _cs = cs_pin;
    _rst = rst_pin;
    _cd = cd_pin;
    _led = led_pin;
    _sda = sda_pin;
    _sck = sck_pin;
    _spi_host = spi_host;

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << _cd) | (1ULL << _rst) | (1ULL << _led) | (1ULL << _cs),
        .pull_down_en = 0,
        .pull_up_en = 1
    };
    gpio_config(&io_conf);

    gpio_set_level(_led, 1);        // Turn on LCD backlight
    gpio_set_level(_rst, 1);        // Prevent the hardware reset. Reset on low.

    spi_bus_config_t buscfg = {
        .miso_io_num = -1,          // LCD are typically write-only, unless it has a touch screen.
        .mosi_io_num = 23,     // 23
        .sclk_io_num = 18,     // 18
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    if(_spi_host == -1){
        ESP_LOGW(TAG, "SPI Host unspecified, using VSPI");
        _spi_host = VSPI_HOST;
    }

    esp_err_t ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus: %s", esp_err_to_name(ret));
    }

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .clock_speed_hz = 4000000,           // Clock out at 4 MHz
        .mode = 0,                           // SPI mode 0
        .spics_io_num = 5,              
        .queue_size = 7,                     // We want to be able to queue 7 transactions at a time
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .pre_cb = lcd_cs_active,
        .post_cb = lcd_cs_idle,
    };
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &_spi_device_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device: %s", esp_err_to_name(ret));
    }

    lcd_reset();

    static const uint16_t initilization_cmd[] = {
            LCD_POWER_CONTROL_1, 0x2F8E,   // Power control         
            LCD_POWER_CONTROL_2, 0x000C,  
            LCD_DISPLAY_CONTROL, 0x0021,           
            LCD_VCOM_OTP_1, 0x0006,      
            LCD_VCOM_OTP_1, 0x0005,     
            LCD_FURTHER_BIAS_CURRENT_SETTING, 0x057F,        
            LCD_VCOM_OTP_2, 0x89A1,     
            LCD_OSCILLATOR, 0x0001,     
            LCD_DELAY, 100,       
            LCD_VCOM_OTP_2, 0x80B0,   
            LCD_DELAY, 30, 
            LCD_VCOM_OTP_2, 0xFFFE,
            LCD_DISPLAY_CONTROL, 0x0223,
            LCD_DELAY, 30, 
            LCD_DISPLAY_CONTROL, 0x0233,
            LCD_DRIVER_OUTPUT_CONTROL, 0x2183,
            LCD_ENTRY_MODE, 0x6838,    //0x03, 0x6830 Original  0x6838 Direction flip
            0x2F, 0xFFFF,
            0x2C, 0x8000,
            LCD_FURTHER_BIAS_CURRENT_SETTING, 0x0570,
            LCD_DRIVING_WAVEFORM_CONTROL, 0x0300,
            LCD_FRAME_CYCLE_CONTROL, 0x580C,
            LCD_POWER_CONTROL_3, 0x0609,
            LCD_POWER_CONTROL_4, 0x3100, 
    };
    uint16_t *p = initilization_cmd;
    uint16_t size = sizeof(initilization_cmd) / sizeof(uint16_t);
    while (size > 0) {
        uint16_t cmd = *p++;
        uint16_t d = *p++;
        if (cmd == 0xFFFF){
            vTaskDelay(pdMS_TO_TICKS(d));
        }
        else {
            write_cmd_data(cmd, d); 
        }
        size -= 2;
        // ESP_LOGI(TAG,"CMD: %x DATA: %x", cmd, d);
    }

    ESP_LOGI(TAG, "Initialization Completed");
}

void lcd_reset(){
    gpio_set_level(_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(_rst, 1);
}

void lcd_flood_screen(uint16_t color){
    write_cmd_8(0x22);
    for(uint8_t x = _x_start; x < _x_end; x++){
        for(uint8_t y = _y_start; y < _y_end; y++){
            writeData16(color);
        }
    }

    write_cmd_8(0x22);

    ESP_LOGI(TAG, "Filled screen with %x", color);
}

void lcd_set_window(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2){
    uint8_t XC=0x45,YC=0x44,CC=0x22,RC=0x2E,SC1=0x41,SC2=0x42,MD=0x03,VL=1,R24BIT=0;
    //TODO: Refactor this
    _x_start = x1;
    _x_end = x2;
    _y_start = y1;
    _y_end = y2;
    write_cmd_8(XC);
    write_data_8(x2 - 1);
    write_data_8(x1);
    write_cmd_8(YC);
    write_data_8(y2 - 1);
    write_data_8(y1);
    write_cmd_8(0x21);
    write_data_8(x1);
    write_data_8(y1);
    write_cmd_8(CC);
}

static void write_cmd_data(uint16_t cmd, uint16_t data){
    //cmd mode
    gpio_set_level(_cd, 0);
    spi_device_transmit(_spi_device_handle, &(spi_transaction_t){
        .length = 8, 
        .tx_data = { (uint8_t) cmd }, 
        .flags = SPI_TRANS_USE_TXDATA 
    });
    
    //data mode
    gpio_set_level(_cd, 1);
    spi_device_transmit(_spi_device_handle, &(spi_transaction_t){
        .length = 16, 
        .tx_data = { (uint8_t) (data >> 8), (uint8_t) data }, 
        .flags = SPI_TRANS_USE_TXDATA 
    });
}

static void lcd_cs_active(spi_transaction_t *t) {
    gpio_set_level(5, 0);
}

static void lcd_cs_idle(spi_transaction_t *t) {
    gpio_set_level(5, 1);
}
#define LEFT 0
#define RIGHT 9999
#define CENTER 9998
// TODO: text printer
// size_t Print(uint8_t *st, int16_t x, int16_t y)
// {
// 	int16_t pos;
// 	uint16_t len;
// 	const char * p = st;
// 	size_t n = 0;
//     uint8_t text_size = 1;
// 	if (x == CENTER || x == RIGHT) 
// 	{
// 		len = strlen(st) * 6 * text_size;		
// 		pos = (130 - len); 
// 		if (x == CENTER)
// 		{
// 			x = pos/2;
// 		}
// 		else
// 		{
// 			x = pos - 1;
// 		}
// 	}
//     // Set_Text_Cousur(x, y);
// 	while(1)
// 	{
// 		unsigned char ch = *(p++);//pgm_read_byte(p++);
// 		if(ch == 0)
// 		{
// 			break;
// 		}
// 		if(write(ch))
// 		{
// 			n++;
// 		}
// 		else
// 		{
// 			break;
// 		}
// 	}	
// 	return n;
// }