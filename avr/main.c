
#include <avr/io.h>          // (1)

#include <util/delay.h>
#include "uart_conf.h"

#include <stdlib.h>
#include <avr/interrupt.h>

#include <stdio.h>

#include "lcd-routines.h"
#include "led_strip.h"

#include "ds18x20.h"
#include "onewire.h"

int uart_putc(unsigned char c);
void ws2801_set_all(int r, int g, int b);
void ws2801_use_preset(uint8_t preset_idx);
void ws2801_scroll(void);
void ws2801_set_slider(int value);
void disp_show_buf(char *buf);
void ws2801_repeat_buffer(ledidx_t startIndex);

#define WS2801_PRESET_MAX 11
uint8_t ws2801_preset = 0;
uint8_t ws2801_scroll_speed=0, ws2801_scroll_timer=0, ws2801_draw = 0;

//char uart_send_buf[40];
//char* uart_send_buf_ptr = &uart_send_buf;

char relay_set = 0;
char relay_reset = 0;
char relay_timer = 0;


char disp_buf[32];
char disp_tmp_buf[32]={'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x'};
char disp_set = 0;
char uart_mode = '\0';

uint8_t ws2801_buf[WS2801_STRIP_LEN * 3 + 1];

ledidx_t recv_len = 0;
uint8_t recv_tmpa = 0, recv_tmpb = 0;


ISR(USART_RXC_vect) {
  char input = UDR;
  if (input == 0x1b) { uart_mode = 0; recv_len = 0; recv_tmpa = 0; recv_tmpb = 0; return; }
  switch(uart_mode) {
  case 0:
    uart_mode = input;
    break;
  case 0x11: //device control1 = open door
    if (input > 0) {
      relay_timer = input;
      relay_set = 1; relay_reset = 0;
    } else {
      relay_reset = 1; relay_set = 0;
    }
    uart_mode = 0xff;
    break;
  case 0x12:  // device control2 = set backlight
    if (input == '1') PORTC |= (1<<PC3);
    else if (input == '0') PORTC &= ~(1<<PC3);
    uart_mode = 0xff;
    break;
  case 0x02: //start of text
    disp_buf[recv_len-1] = input;
    if (recv_len == 32) {
      uart_mode = 0xff;
      disp_set = 1;
    }
    break;
  case 0x01: //start of transmission = set ws2801 individually
    //uart_putc('k'); uart_putc(WS2801_STRIP_LEN);
    if (recv_tmpa == 1) { //last byte was 0xFE - handle escape sequence
      if (input == 0xFD) ws2801_buf[recv_len-1] = 0x1b;
      else if (input == 0xFE) ws2801_buf[recv_len-1] = 0xFE;
      else if (input == 0xFC) { //end of text - repeat everything
        ws2801_repeat_buffer(recv_len-1);
        uart_mode = 0xff;
        ws2801_draw = 1;
        uart_putc('D');
      }
      recv_tmpa = 0;
    } else if (input == 0xFE) { //this byte is 0xFE - ignore and enable escape mode
      recv_tmpa = 1;
      recv_len --;
    } else {  // normal byte
      ws2801_buf[recv_len-1] = input;
      if (recv_len == 3*WS2801_STRIP_LEN) {
        uart_mode = 0xff;
        ws2801_draw = 1;
        uart_putc('d');
      }
    }
    break;
  case 0x14: //device control4 = set scroll speed
    ws2801_scroll_speed = input;
    ws2801_scroll();
    ws2801_draw = 1;
    uart_mode = 0xff;
    break;
  case 0x07: //bell = sound the buzzer
    if (recv_len==1) recv_tmpa=input;
    else {
      for(;input!=0;input--){
        PORTB|= (1 << PB3); for(recv_tmpb=recv_tmpa;recv_tmpb!=0;recv_tmpb--) _delay_us(8);
        PORTB&= ~(1 << PB3); for(recv_tmpb=recv_tmpa;recv_tmpb!=0;recv_tmpb--) _delay_us(8);
      }
      uart_mode = 0xff;
    }
    break;
  default:
    uart_mode = 0xff;
    uart_putc('?');
    break;
  }
  recv_len++;
}


int main (void) {            // (2)
   ledidx_t i;

   DDRB  = 0xFF;  // Port B: 1 = output
   PORTB = 0x01;  //bootup 1

   //_delay_ms(1000);

   // Initialize LCD Display
   DDRC |= (1<<PC1) | (1<<PC3); //PC1 = R/W, PC3 = Backlight control
   PORTC &= ~(1<<PC1);
   //Switch Backlight on:
   PORTC |= (1<<PC3);

   _delay_ms(30); lcd_init();

   PORTB = 0x02;  //bootup 2
   _delay_ms(100);
   lcd_string_P(PSTR("blinkylight 0.3 "));
   lcd_setcursor(0,2);
   lcd_string_P(PSTR("Booting ...     "));
   //PORTB = 0x03;  //bootup 3
   //_delay_ms(1000);

   uart_init();
   uart_putc('p'); uart_putc('w'); uart_putc('r'); uart_putc('O'); uart_putc('N'); uart_putc('\n');
   //PORTB = 0x04;  //bootup 4



   //PORTB = 0x05;  //bootup 5

   // Enable Interrupts
   sei();
   lcd_setcursor(0,2);
   lcd_string_P(PSTR("Init LED        "));
   PORTB = 0x06;  //bootup 6

   // Initialize WS2801 LED
   // muss vor ws2801_init stehen, da dieser PA1 und PA2 als output schaltet
   DDRA  = 0x00; // Port A: 0 = input
   PORTA = 0x00;  //        0 = pull-ups off
   ws2801_init();

   //PORTB = 0x0a;  //bootup a

   // Startup Blinky on WS2801 LED
   ws2801_use_preset(2);
   //PORTB = 0x0b;  //bootup b
  for(i=0;i<50;i++){_delay_us(890);PORTB ^= (1 << PB3);}
  _delay_ms(950);
   //PORTB = 0x0c;  //bootup c
   ws2801_use_preset(6);

  //for(i=0;i<180;i++){_delay_us(290);PORTB ^= (1 << PB3);}

   //_delay_ms(1000);
   ws2801_set_all(0,0,0);


   PORTB = 0x00;  //bootup d
   lcd_setcursor(0,2);
   lcd_string_P(PSTR("Boot complete  "));
   _delay_ms(500);

   //Switch Backlight off:
   PORTC &= ~(1<<PC3);

   // Enter main loop
   uint8_t dezisek = 0;
#define DEZISEKTHRES 4
   while(1) {                // (5)
     /* "leere" Schleife*/   // (6)
     _delay_ms(25);
     //pb_scroll <<= 1;
     //if (pb_scroll == 0b00010000) pb_scroll = 0b00000001;
     //PORTB &= 0b11110000;
     //PORTB |= pb_scroll;
     PORTB ^= (1<<PB2);
     if (dezisek > DEZISEKTHRES) {
       if (relay_timer > 0) {
         relay_timer --;
         if (relay_timer == 0) relay_reset = 1;
         else {
           PORTB ^= ( 1 << PB5 )|(1<<PB6)|(1<<PB7);
         }
       }
       ws2801_draw = 1; //auto redraw every 1/10 second
     }
     dezisek++;
     if (disp_set) {
       lcd_clear();
       lcd_home();

       for(i=0;i<16;i++)lcd_data(disp_buf[i]);
       lcd_setcursor(0,2);
       for(;i<32;i++)lcd_data(disp_buf[i]);

       disp_set = 0;
       _delay_ms(250);
     }
     if (relay_set) {
       PORTB |= (1<<PB4);
       PORTB |= (1<<PB5)|(1<<PB6)|(1<<PB7);
       relay_set = 0;
     }
     if (relay_reset) {
       PORTB &= ~(1<<PB4);
       PORTB &= ~((1<<PB5)|(1<<PB6)|(1<<PB7));
       relay_reset = 0; relay_timer = 0;
     }
     if (ws2801_scroll_speed != 0 && uart_mode != 0x01) {
       if (ws2801_scroll_timer == ws2801_scroll_speed) {
         ws2801_scroll_timer = 0;
         ws2801_scroll();
         ws2801_draw = 1;
       }
       ws2801_scroll_timer+=1;
     }
     if (ws2801_draw && uart_mode != 0x01) {
       ws2801_draw_buffer(&ws2801_buf[0]);
       ws2801_draw = 0;
     }
     if (PINA & (1<<PA7)) {
       ws2801_preset++;
       if (ws2801_preset > WS2801_PRESET_MAX) ws2801_preset = 0;
       ws2801_use_preset(ws2801_preset);
       uart_putc('5');
       _delay_ms(200);
     }
     if (PINA & (1<<PA6)) {
       ws2801_preset = 0;
       ws2801_scroll_speed = 0;
       ws2801_use_preset(ws2801_preset);
       uart_putc('4');
     }
     if (PINA & (1<<PA5)) {
       ws2801_scroll_speed--;
       if (ws2801_scroll_speed > 70) ws2801_scroll_speed = 70;
       ws2801_set_slider(ws2801_scroll_speed);
       for(i=0;i!=ws2801_scroll_speed;i++) _delay_ms(1);
       ws2801_scroll_timer = 0;
       uart_putc('3');
     }
     if (PINA & (1<<PA4)) {
       uart_putc('2');
       PORTC ^= (1<<PC3);
       while (PINA & (1<<PA4)) {_delay_us(690);PORTB ^= (1 << PB3);}
       uint8_t sensor_id[OW_ROMCODE_SIZE];
       uint8_t diff = OW_SEARCH_FIRST;
       ow_reset();
       DS18X20_find_sensor(&diff, &sensor_id[0]);
       if (diff == OW_PRESENCE_ERR)
           strcpy_P(&disp_tmp_buf[0], PSTR("Err:Presence     "));
       else if (diff == OW_DATA_ERR)
           strcpy_P(&disp_tmp_buf[0], PSTR("Err:Data         "));
       else {
         if ( DS18X20_start_meas( DS18X20_POWER_PARASITE, NULL ) == DS18X20_OK) {
           _delay_ms( DS18B20_TCONV_12BIT );
           int16_t decicelsius;
           if ( DS18X20_read_decicelsius( &sensor_id[0], &decicelsius) == DS18X20_OK ) {
             disp_tmp_buf[0]='T'; disp_tmp_buf[1]='e'; disp_tmp_buf[2]='m'; disp_tmp_buf[3]='p'; disp_tmp_buf[4]=':'; disp_tmp_buf[5]=' ';
             DS18X20_format_from_decicelsius( decicelsius, &disp_tmp_buf[6], 8 );
           } else {
             strcpy_P(&disp_tmp_buf[0], PSTR("Err: Read        "));
           }
         } else {
             strcpy_P(&disp_tmp_buf[0], PSTR("Err: StartMeasure"));
         }
       }
       sprintf(&disp_tmp_buf[16], "%d bytes recv.", recv_len);
       disp_show_buf(&disp_tmp_buf[0]);
       for(i=0;i<50;i++) {_delay_us(300);PORTB ^= (1 << PB3);}
     }
     if (PINA & (1<<PA3)) {
       relay_set = 1;
       relay_timer = 10;
       uart_putc('1');
     }
     //uart_putc('+');
//     uart_putc('\n');
   }                         // (7)
   /* wird nie erreicht */
   return 0;                 // (8)
}

#define WS2801_SET_NEXT_COLOR(r,g,b) ws2801_buf[i++] = r; ws2801_buf[i++] = g; ws2801_buf[i++] = b;
void ws2801_set_all(int r, int g, int b) {
    ws2801_buf[0] = r; ws2801_buf[1] = g; ws2801_buf[2] = b;
    ws2801_repeat_buffer(3);
}
void ws2801_use_preset(uint8_t preset_idx) {
  ledidx_t i = 0;
  switch(preset_idx) {
  case 0:
    ws2801_set_all(0, 0, 0);
    break;
  case 1:
    ws2801_set_all(1, 1, 1);
    break;
  case 2:
    WS2801_SET_NEXT_COLOR(150,110,70)
    WS2801_SET_NEXT_COLOR(150,110,70)
    WS2801_SET_NEXT_COLOR(150,110,70)
    WS2801_SET_NEXT_COLOR(150,110,70)
    WS2801_SET_NEXT_COLOR(200,170,10)
    ws2801_repeat_buffer(i);
    break;
  case 3:
    WS2801_SET_NEXT_COLOR(255,0,0)
    WS2801_SET_NEXT_COLOR(255,160,0)
    WS2801_SET_NEXT_COLOR(255,255,0)
    WS2801_SET_NEXT_COLOR(0,255,0)
    WS2801_SET_NEXT_COLOR(0,255,190)
    WS2801_SET_NEXT_COLOR(0,0,255)
    WS2801_SET_NEXT_COLOR(255,0,190)
    ws2801_repeat_buffer(i);
    break;
  case 4:
    ws2801_set_all(255,200,100);
    break;
  case 5:
    ws2801_set_all(255,255,255);
    break;
  case 6:
    ws2801_set_all(255,0,0);
    break;
  case 7:
    ws2801_set_all(255,200,0);
    break;
  case 8:
    ws2801_set_all(0,255,0);
    break;
  case 9:
    ws2801_set_all(0,0,255);
    break;
  case 10:
    ws2801_set_all(255,0,255);
    break;
  case 11:
    ws2801_set_all(100,30,15);
    break;

  }
  ws2801_draw_buffer(&ws2801_buf[0]);
}

int uart_putc(unsigned char c)
{
    while (!(UCSRA & (1<<UDRE)))  /* warten bis Senden moeglich */
    {
    }

    UDR = c;                      /* sende Zeichen */
    return 0;
}

void disp_show_buf(char *buf) {
    uint8_t i;
    lcd_clear();
    lcd_home();
    for(i=0;i<16;i++)lcd_data(buf[i]);
    lcd_setcursor(0,2);
    for(;i<32;i++)lcd_data(buf[i]);
}

void ws2801_set_slider(int value) {
    ledidx_t i;
    for(i = 0; i != WS2801_STRIP_LEN; i++) {
      _ws2801_send_byte(i>=value ? 255:0);
      _ws2801_send_byte(i>=value ? 0:255);
      _ws2801_send_byte(i==value ? 255:0);
    }

}

void ws2801_repeat_buffer(ledidx_t startIndex) {
  for(int i = startIndex-1; i != 3*WS2801_STRIP_LEN; i++) {
    ws2801_buf[i] = ws2801_buf[i-startIndex];
  }
}

void ws2801_scroll() {
  int tmpR = ws2801_buf[0], tmpG = ws2801_buf[1], tmpB = ws2801_buf[2];
  for(ledidx_t i = 0; i != WS2801_STRIP_LEN*3-3; i+=3) {
    ws2801_buf[i] = ws2801_buf[i+3];
    ws2801_buf[i+1] = ws2801_buf[i+4];
    ws2801_buf[i+2] = ws2801_buf[i+5];
  }
  ws2801_buf[(WS2801_STRIP_LEN-1)*3] = tmpR;
  ws2801_buf[(WS2801_STRIP_LEN-1)*3+1] = tmpG;
  ws2801_buf[(WS2801_STRIP_LEN-1)*3+2] = tmpB;

}

