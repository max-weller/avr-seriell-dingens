
#include <avr/io.h>          // (1)

#include <util/delay.h>
#include "uart_conf.h"

#include <stdlib.h>
#include <avr/interrupt.h>

#include <stdio.h>

#include "lcd-routines.h"
#include "led_strip.h"

int uart_putc(unsigned char c)
{
    while (!(UCSRA & (1<<UDRE)))  /* warten bis Senden moeglich */
    {
    }

    UDR = c;                      /* sende Zeichen */
    return 0;
}

//char uart_send_buf[40];
//char* uart_send_buf_ptr = &uart_send_buf;

char relay_set = 0;
char relay_reset = 0;
char relay_timer = 0;


char disp_buf[33];
char disp_buf_ctr = 0;
char disp_set = 0;
char uart_mode = '\0';

char ws2801_r=0, ws2801_g=0, ws2801_b=0, ws2801_idx=0;
char ws2801_buf[WS2801_STRIP_LEN * 3];

ISR(USART_RXC_vect) {
  char input = UDR;
  if (input == 0x1b) { uart_mode = 0; }
  else {
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
    uart_mode = 0;
    break;
  case 0x12:  // device control2 = set backlight
    if (input == '1') PORTC |= (1<<PC3);
    else if (input == '0') PORTC &= ~(1<<PC3);
    uart_mode = 0;
    break;
  case 0x02: //start of text
    disp_buf[disp_buf_ctr] = input;
    disp_buf_ctr++;
    if (disp_buf_ctr == 32) {
      uart_mode = 0;
      disp_buf_ctr = 0;
      disp_set = 1;
    }
    break;
  case 0x13:   // device control3 = set all ws2801 to same color
    if (ws2801_idx==0) ws2801_r=input;
    else if (ws2801_idx==1) ws2801_g=input;
    else {
      ws2801_b=input;
      ws2801_draw_all(ws2801_r, ws2801_g, ws2801_b);
      uart_mode = 0; ws2801_idx=0;
      break;
    }
    ws2801_idx++;
    break;
  case 0x01: //start of transmission = set ws2801 individually
    //uart_putc('k'); uart_putc(WS2801_STRIP_LEN);
    if (ws2801_idx < 3*WS2801_STRIP_LEN) {
      ws2801_buf[ws2801_idx] = input;
      ws2801_idx++;
    } else {
      uart_mode = 0;
      ws2801_idx = 0;
      ws2801_draw_buffer(&ws2801_buf);
      uart_putc('d');
    }
    break;
  default:
    uart_mode = 0;
    uart_putc('?');
    break;
  }
  }
}


int main (void) {            // (2)
   uint8_t i;

   uart_init();

   DDRC |= (1<<PC1) | (1<<PC3); //PC1 = R/W, PC3 = Backlight control
   PORTC &= ~(1<<PC1);

   //Backlight on:
   PORTC |= (1<<PC3);

   lcd_init();

   sei();

   lcd_string("Eile mit Weile");

   ws2801_init();
   for(i = 0; i< WS2801_STRIP_LEN*3; i+=3) {
     ws2801_buf[i] = 0x3b; ws2801_buf[i+1] = 0x17; ws2801_buf[i+2] = 0x0f;
     if (i%5 == 0) { ws2801_buf[i]=0x5b; ws2801_buf[i+2]=0; }
   }
   ws2801_draw_buffer(&ws2801_buf);

   DDRB  = 0xFF;             // (3)
   char pb_scroll = 0x01;             // (4)
   while(1) {                // (5)
     /* "leere" Schleife*/   // (6)
     _delay_ms(100);
     pb_scroll <<= 1;
     if (pb_scroll == 0b00010000) pb_scroll = 0b00000001;
     PORTB &= 0b11110000;
     PORTB |= pb_scroll;
     if (relay_timer > 0) {
       relay_timer --;
       if (relay_timer == 0) relay_reset = 1;
       else {
         PORTB ^= ( 1 << PB5 )|(1<<PB6)|(1<<PB7);
       }
     }
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
     //uart_putc('+');
//     uart_putc('\n');
   }                         // (7)
   /* wird nie erreicht */
   return 0;                 // (8)
}

