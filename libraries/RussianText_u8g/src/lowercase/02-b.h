// русская 'б'

// void b(uint8_t x, uint8_t y); 


void b(uint8_t x, uint8_t y) { 
    // б

	//  .....  1
	//  .....  2  
    //  ....0  3
    //  .0000  4
    //  0....  5
    //  0000.  6
    //  0...0  7
    //  .000.  8
    //  .....  9

    // вывод построчно
    
    // первая строка
    // u8g.drawPixel(x+0,y);
    // u8g.drawPixel(x+1,y);
    // u8g.drawPixel(x+2,y);
    // u8g.drawPixel(x+3,y);
    // u8g.drawPixel(x+4,y);

    // вторая строка
    // u8g.drawPixel(x+0,y+1);
    // u8g.drawPixel(x+1,y+1);
    // u8g.drawPixel(x+2,y+1);
    // u8g.drawPixel(x+3,y+1);
    // u8g.drawPixel(x+4,y+1);

    // третья строка
    // u8g.drawPixel(x+0,y+2);
    // u8g.drawPixel(x+1,y+2);
    // u8g.drawPixel(x+2,y+2);
    // u8g.drawPixel(x+3,y+2);
    u8g.drawPixel(x+4,y+2);

    // четвёртая строка
    // u8g.drawPixel(x+0,y+3);
    u8g.drawPixel(x+1,y+3);
    u8g.drawPixel(x+2,y+3);
    u8g.drawPixel(x+3,y+3);
    u8g.drawPixel(x+4,y+3);

    // пятая строка
    u8g.drawPixel(x+0,y+4);
    // u8g.drawPixel(x+1,y+4);
    // u8g.drawPixel(x+2,y+4);
    // u8g.drawPixel(x+3,y+4);
    // u8g.drawPixel(x+4,y+4);
    
    // шестая строка
    u8g.drawPixel(x+0,y+5);
    u8g.drawPixel(x+1,y+5);
    u8g.drawPixel(x+2,y+5);
    u8g.drawPixel(x+3,y+5);
    // u8g.drawPixel(x+4,y+5);
    
    // седьмая строка
    u8g.drawPixel(x+0,y+6);
    // u8g.drawPixel(x+1,y+6);
    // u8g.drawPixel(x+2,y+6);
    // u8g.drawPixel(x+3,y+6);
    u8g.drawPixel(x+4,y+6);
    
    // восьмая строка
    // u8g.drawPixel(x+0,y+7);
    u8g.drawPixel(x+1,y+7);
    u8g.drawPixel(x+2,y+7);
    u8g.drawPixel(x+3,y+7);
    // u8g.drawPixel(x+4,y+7);
   
    // девятая строка
    // u8g.drawPixel(x+0,y+8);
    // u8g.drawPixel(x+1,y+8);
    // u8g.drawPixel(x+2,y+8);
    // u8g.drawPixel(x+3,y+8);
    // u8g.drawPixel(x+4,y+8);

}