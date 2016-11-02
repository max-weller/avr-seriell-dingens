
#include <avr/io.h>          // (1)

#include <util/delay.h>

#include "uart_conf.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "softuart.h"

#include "lcd-routines.h"

#include "ds18x20.h"
#include "onewire.h"



int uart_putc(unsigned char c)
{
    while (!(UCSRA & (1<<UDRE)))  /* warten bis Senden moeglich */
    {
    }

    UDR = c;                      /* sende Zeichen */
    return 0;
}

char disp_buf[32];
char disp_tmp_buf[32]={'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x'};
uint8_t relay_timer;

int main (void) {            // (2)
   uint8_t i;

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

   softuart_init();
   softuart_turn_rx_on(); /* redundant - on by default */

   sei();

    softuart_puts_P( "\r\nSoftuart Demo-Application\r\n" );    // "implicit" PSTR

   lcd_setcursor(0,2);
   lcd_string_P(PSTR("Init LED        "));
   PORTB = 0x06;  //bootup 6

   DDRA  = 0x00; // Port A: 0 = input
   PORTA = 0x00;  //        0 = pull-ups off

   //PORTB = 0x0a;  //bootup a

   // Startup Blinky on WS2801 LED
   //PORTB = 0x0b;  //bootup b
  for(i=0;i<50;i++){_delay_us(890);PORTB ^= (1 << PB3);}
  _delay_ms(950);
   //PORTB = 0x0c;  //bootup c

  //for(i=0;i<180;i++){_delay_us(290);PORTB ^= (1 << PB3);}


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
     _delay_ms(25);
     //pb_scroll <<= 1;
     //if (pb_scroll == 0b00010000) pb_scroll = 0b00000001;
     //PORTB &= 0b11110000;
     //PORTB |= pb_scroll;
     PORTB ^= (1<<PB2);
     if (dezisek > DEZISEKTHRES) {
       if (relay_timer > 0) {
         relay_timer --;
         if (relay_timer == 0) {
             PORTB &= ~(1<<PB4);
             PORTB &= ~((1<<PB5)|(1<<PB6)|(1<<PB7));
         } else {
           PORTB ^= ( 1 << PB5 )|(1<<PB6)|(1<<PB7);
         }
       }
     }
     dezisek++;
     if (softuart_kbhit()) {
       uint8_t cmdbyte = softuart_getchar();
       switch(cmdbyte) {
         case 0x02:
           // set lcd display
           lcd_clear();
           lcd_home();
           for(i=0; i<32; i++) {
             lcd_data(softuart_getchar());
             if (i == 16) lcd_setcursor(0,2);
           }
           break;
         case 0x11:
           relay_timer = softuart_getchar();
           PORTB |= (1<<PB4);
           PORTB |= (1<<PB5)|(1<<PB6)|(1<<PB7);
           break;

         case 0x12:  // device control2 = set backlight
           i = softuart_getchar();
           if (i == '1') PORTC |= (1<<PC3);
           else if (i == '0') PORTC &= ~(1<<PC3);
           break;
         case 0x13: {

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
           disp_tmp_buf[16] = 0;
           softuart_puts(disp_tmp_buf);
           break;
         }
       }
     }
     if (PINA & (1<<PA7)) {
       softuart_putchar('5');
       _delay_ms(200);
     }
     if (PINA & (1<<PA6)) {
       softuart_putchar('4');
     }
     if (PINA & (1<<PA5)) {
       softuart_putchar('3');
     }
     if (PINA & (1<<PA4)) {
       softuart_putchar('2');
     }
     if (PINA & (1<<PA3)) {
       softuart_putchar('1');
     }
   }                         // (7)
   /* wird nie erreicht */
   return 0;                 // (8)
}
