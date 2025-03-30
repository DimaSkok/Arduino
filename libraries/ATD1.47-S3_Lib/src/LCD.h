#pragma once

#include <stdint.h>
#include <string.h>

#if defined(__has_include)
#if __has_include(<lvgl.h>)
#include <lvgl.h>
#define USE_LVGL
#endif
#else
#error "__has_include not work"
#endif

#define LCD_WIDTH (320)
#define LCD_HEIGHT (172)

class LCD {
    private:
        uint64_t clock;
        void initSPI()  ;
        void write_spi(uint8_t data) ;
        void write_spi(uint8_t *data, size_t len) ;

        void write_cmd(uint8_t cmd) ;
        void write_cmd(uint8_t cmd, uint8_t *data, int len) ;
        void initST7789() ;
        
        void setWindow(int x_start, int y_start, int x_end, int y_end) ;

    public:
        LCD();
        void begin(uint32_t speed = 40E6) ;
        void drawBitmap(int x_start, int y_start, int x_end, int y_end, uint16_t* color_data) ;

        uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) ;
        uint32_t color24to16(uint32_t color888) ;

        void drawPixel(uint16_t x, uint16_t y, uint16_t color) ;
        void fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) ;
        void off() ;
        void on() ;
        void fillScreen(uint16_t color) ;
        void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) ;
        void drawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) ;
        void drawRectAngle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color) ;
        void drawTriangle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color) ;
        void drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) ;
        void fillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) ;
        void drawRoundRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t color) ;
        void drawArrow(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t w,uint16_t color) ;
        void fillArrow(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t w,uint16_t color) ;

#if __has_include(<lvgl.h>)
        void useLVGL() ;
        void loop() ;
#endif

};

extern LCD Display;
