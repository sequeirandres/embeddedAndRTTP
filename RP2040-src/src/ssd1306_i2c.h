#ifndef _HEADER_SSD1306_I2C_
#define _HEADER_SSD1306_I2C_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

// for SSD1306.h
//#include "raspberry26x32.h"
#include "ssd1306_font.h"


/* The SSD1306 is an OLED/PLED driver chip, capable of driving displays up to
   128x64 pixels.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   ---- GPIO PICO_DEFAULT_I2C_SDA_PIN (on Pico this is GP4 (pin 6)) -> SDA on display
      board
   ---- GPIO PICO_DEFAULT_I2C_SCL_PIN (on Pico this is GP5 (pin 7)) -> SCL on
   
   NEW CONFIG DEPENDE DE LOS PINES DISPONIBLES
   
   GPIO PICO_DEFAULT_I2C_SDA_PIN (on Pico this is GP4 (pin 4)) -> SDA on display
   board
   GPIO PICO_DEFAULT_I2C_SCL_PIN (on Pico this is GP5 (pin 5)) -> SCL on
   
   display board
   3.3v (pin 36) -> VCC on display board
   GND (pin 38)  -> GND on display board
*/

// Define the size of the display we have attached. This can vary, make sure you
// have the right size defined or the output will look rather odd!
// Code has been tested on 128x32 and 128x64 OLED displays
#define SSD1306_HEIGHT              64
#define SSD1306_WIDTH               128

//#define SSD1306_I2C_ADDR          _u(0x78)
#define SSD1306_I2C_ADDR	 _u(0x3C)  /// solo funciona con esta direccion

// DEFINE GPIO PIN_SDA & PIN_SCL  - CUSTOM 

#define SSD1306_PICO_I2C_SDA_PIN	2
#define SSD1306_PICO_I2C_SCL_PIN	3
#define SSD1306_GPIO_FUNC_I2C 		i2c1  

// GPIO PIN_SDA & PIN_SCL - DEFAULT 
//#define SSD1306_PICO_I2C_SDA_PIN	4
//#define SSD1306_PICO_I2C_SCL_PIN	5
// #define SSD1306_GPIO_FUNC_I2C	i2c0  


// 400 is usual, but often these can be overclocked to improve display response.
// Tested at 1000 on both 32 and 84 pixel height devices and it worked.
#define SSD1306_I2C_CLK             400
//#define SSD1306_I2C_CLK             1000


// commands (see datasheet)
#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)										// Altura de cada fila
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)     // La cantidad de filas que hay 64/8 = 8 filas
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)		 // Cantidad total de caracteres 8*128	

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)


// Define struct area 
typedef struct //render_area
{
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int buflen;
}SSD1306_t;

//void calc_render_area_buflen(struct render_area *area) ;
//void calc_render_area_buflen(SSD1306_t *area) ;

void uint_to_char(char *value_string,uint32_t value);

void calc_render_area_buflen(SSD1306_t *area) ;

void SSD1306_send_cmd(uint8_t cmd) ;

void SSD1306_send_cmd_list(uint8_t *buf, int num);

void SSD1306_send_buf(uint8_t buf[], int buflen);

void SSD1306_init();

void SSD1306_scroll(bool on) ;

//void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str) ;

//void render(uint8_t *buf, struct render_area *area);

void render(uint8_t *buf,SSD1306_t *area);

static void SetPixel(uint8_t *buf, int x,int y, bool on);

static void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) ;

static inline int GetFontIndex(uint8_t ch) ;

static uint8_t reverse(uint8_t b) ;

static void FillReversedCache(void) ;

static void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) ;

void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str) ;

void SSD1306_gpio_set(uint8_t sda_pin, uint8_t scl_pin) ;

void SSD1306_clear() ;

void SSD1306_Write(uint8_t col,uint8_t fil,char *str, uint8_t strlen);



#ifdef __cplusplus
}
#endif


#endif
