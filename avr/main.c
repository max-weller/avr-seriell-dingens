
#include <avr/io.h>          // (1)
#include "register_bitfields.h"

#include <util/delay.h>
#include "uart_conf.h"

#include <stdlib.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <string.h>

#include "lcd-routines.h"
#include "led_strip.h"

#include "ds18x20.h"
#include "onewire.h"

#include "../Irmp/irmp.h"

//#include "softuart.h"

/******************* port definitions ***************/

// PORT B
#define LED_RED2_PORT REGISTER_BIT(PORTB, 7)
#define LED_RED2_DDR REGISTER_BIT(DDRB, 7)

#define LED_RED3_PORT REGISTER_BIT(PORTB, 6)
#define LED_RED3_DDR REGISTER_BIT(DDRB, 6)

#define LED_GREEN1_PORT REGISTER_BIT(PORTB, 5)
#define LED_GREEN1_DDR REGISTER_BIT(DDRB, 5)

#define RELAY_PORT REGISTER_BIT(PORTB, 4)
#define RELAY_DDR REGISTER_BIT(DDRB, 4)

// 5=g1 4=g2 3=g3 2=g4 1=g5
#define LED_GREEN4_PORT REGISTER_BIT(PORTB, 2)
#define LED_GREEN4_DDR REGISTER_BIT(DDRB, 2)

#define LED_GREEN5_PORT REGISTER_BIT(PORTB, 1)
#define LED_GREEN5_DDR REGISTER_BIT(DDRB, 1)
/*
#define RS485_DE_PORT REGISTER_BIT(PORTB, 0)
#define RS485_DE_DDR REGISTER_BIT(DDRB, 0)
*/
// PORT C
#define BACKLIT_PORT REGISTER_BIT(PORTC, 3)
#define BACKLIT_DDR REGISTER_BIT(DDRC, 3)

#define IR_RCV_PIN REGISTER_BIT(PIND, 2)   // INT 0
#define IR_RCV_DDR REGISTER_BIT(DDRD, 2)

#define LCD_RW_PORT REGISTER_BIT(PORTC, 1)
#define LCD_RW_DDR REGISTER_BIT(DDRC, 1)


// PORT D

//#define MOTION_DET_PIN REGISTER_BIT(PIND, 3)
//#define MOTION_DET_PULLUP REGISTER_BIT(PORTD, 3)
//#define MOTION_DET_DDR REGISTER_BIT(DDRD, 3)

//#define ONEWIRE_PORT REGISTER_BIT(PORTD, 4)
//#define ONEWIRE_DDR REGISTER_BIT(DDRD, 4)

#define BELL_INPUT_PORT REGISTER_BIT(PORTB, 0)
#define BELL_INPUT_DDR REGISTER_BIT(DDRB, 0)

#define BUZZER_DDR REGISTER_BIT(DDRD, 4)


/******** configuration / macros for rs485 receiver+transmitter */
#define RS485_BUFLEN 37

#define rs485_start_sending() do{}while(0)
#define begin_critical_section() //__asm("cli")
#define end_critical_section() //__asm("sei")
#define rs485_finish_sending() do{  }while(0)


#define rs485_send_byte(bt) putchar(bt)

//#define sendByteCk(bt) TXREG=bt; cksum^=bt; while(!PIR1bits.TXIF);

//const char [] MY_ADDR = {'L', 'E', 'D', 'S'};
#define MY_ADDR 0x41

#include "../rs485_receiver.c"


#define initialize_output(ddr) (ddr = 1)
#define initialize_input(ddr) (ddr = 0)






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

uint8_t relay_stimer = 0; //seconds
uint8_t backlit_stimer = 0; //seconds
uint8_t redraw_mstimer = 0; //milliseconds
uint16_t onewire_mstimer = 0; //milliseconds
uint8_t buzzer_mstimer = 0; //milliseconds
uint8_t key_hmstimer = 0; //milliseconds
uint16_t bell_mstimer = 0; //milliseconds
uint8_t keyspressed = 0, keydownevent = 0, keyupevent = 0;
#define KEYS_MASK ((1<<PA7)|(1<<PA6)|(1<<PA5)|(1<<PA4)|(1<<PA3)|(1<<PA0))
#define KEY_OK      (1<<PA7)
#define KEY_PREV    (1<<PA6)
#define KEY_NEXT    (1<<PA5)
#define KEY_UP      (1<<PA4)
#define KEY_CANCEL  (1<<PA3)
#define KEY_DOORBELL (1<<PA0)

#define BUZZ(startfreq, sweepstep, sweeplen, speed) do{ OCR1A=startfreq; TCCR1B = (1<<CS11)|(1<<CS10) | (1<<WGM12); buzzer_sweepstep=sweepstep; buzzer_sweeplen=sweeplen; buzzer_sweepspeed = speed; }while(0)
#define BUZZP(startfreqH, startfreqL, sweepstep, sweeplen, speed) do{ OCR1AH=startfreqH; OCR1AL=startfreqL; TCCR1B = (1<<CS11)|(1<<CS10) | (1<<WGM12); buzzer_sweepstep=sweepstep; buzzer_sweeplen=sweeplen; buzzer_sweepspeed = speed; }while(0)
int8_t buzzer_sweepstep;
uint8_t buzzer_sweeplen;
uint8_t buzzer_sweepspeed;

struct {
  char motion_det_edge : 1;
  char in_menu : 1;
  char onewire_read_meas : 1;
  char key_repeating : 1;
} myflags;

char disp_buf[33];
char disp_buf2[17];
char disp_notice[30];
uint8_t disp_notice_stimer;
uint8_t disp_notice_scroll_mstimer;
uint8_t disp_notice_scroll;

char disp_set = 0;
char uart_mode = '\0';

#define MENU_TXT_LEN 14
#define MENU_MAX_ITEMS 12
char menu_buf[MENU_MAX_ITEMS][MENU_TXT_LEN];
char menu_idx, menu_udata, menu_owner, menu_len;

#define STRIPE2_LENGTH 19

uint8_t ws2801_buf[STRIPE_LENGTH * 3 + STRIPE2_LENGTH * 3 + 1];

char tempbuf[8];

uint16_t t_millis;
uint8_t t_sec;
uint8_t t_min;
uint8_t t_hour;

int16_t decicelsius;

ISR(USART_RXC_vect) {
  recvbyte = UDR;
  receiveRS485();
}



ISR(TIMER0_COMP_vect) {
  //called at 12500Hz
  (void) irmp_ISR();
}

ISR(TIMER1_COMPB_vect) {
  if (buzzer_mstimer-- == 0) {
    if (buzzer_sweeplen==0) {
        TCCR1B=0; //disable timer
     } else {
        buzzer_sweeplen--;
        OCR1A += buzzer_sweepstep;
     }
     buzzer_mstimer = buzzer_sweeplen+3;
   }
}

ISR(TIMER2_COMP_vect) {
  //called every millisecond
  t_millis++;
  // do every millisecond
  if (redraw_mstimer>1) redraw_mstimer--;
  if (onewire_mstimer>1) onewire_mstimer--;
  if (disp_notice_scroll_mstimer>1) disp_notice_scroll_mstimer--;
  bell_mstimer++;
  
  // trigger keyevent for newly pushed buttons
  keydownevent |= (PINA & KEYS_MASK) & ~keyspressed;
  // add newly pushed buttons to the keyspressed mask
  keyspressed |= (PINA & KEYS_MASK);

  if ((PINA & KEYS_MASK) != keyspressed) {  // if keys are released
    if (key_hmstimer == 0) {
      key_hmstimer = 20; //start the release timer
    } else if (key_hmstimer == 1){ //timer fired?
      key_hmstimer = 0; //stop timer
      keyupevent = ~(PINA & KEYS_MASK) & keyspressed;
      keyspressed =  (PINA & KEYS_MASK); //store released keys
    } else {
      key_hmstimer--; //count the timer
    }
  } else {
    key_hmstimer = 0; // stop the timer if keys were not released
  }

  if (t_millis == 1000) {
    t_millis = 0; t_sec++;
    // do every second
    if (relay_stimer>1) relay_stimer--;
    if (backlit_stimer>1) backlit_stimer--;
    if (disp_notice_stimer>1) disp_notice_stimer--;

    if (t_sec == 60) {
      t_sec = 0; t_min++;
      if (t_min == 60) {
        t_min = 0; t_hour++;
        if (t_hour == 60) {
          t_hour = 0;
          // not counting dates (yet)
        }
      }
    }
  }
}

void disp_notice_dismiss(void) {
  disp_notice[0] = 0; disp_notice_stimer = 0;
}

void disp_notice_show(uint8_t time) {
  disp_notice_stimer = time; disp_notice_scroll_mstimer=50;
}

void keypress_backlight(void) {
  if((!BACKLIT_PORT)||(backlit_stimer))backlit_stimer = 6;
  BACKLIT_PORT = 1;
}

void display_homescreen(void) {
   char temperature[8];
   uint8_t i;
   /*DS18X20_format_from_decicelsius( decicelsius, temperature, 8 );
   sprintf_P(disp_buf2, PSTR("%02d:%02d:%02d % 7s"), t_hour, t_min, t_sec, temperature);
   lcd_home();
   for(i=0;i<16;i++)lcd_data(disp_buf2[i]);
   lcd_setcursor(0,2);
   if (disp_notice_stimer>1)
     for(i=0;i<16&&disp_buf[i+disp_notice_scroll];i++)
       lcd_data(disp_notice[i+disp_notice_scroll]);
   else if (t_sec<30)
     for(i=0;i<16;i++)lcd_data(disp_buf[i]);
   else
     for(i=16;i<32;i++)lcd_data(disp_buf[i]);
   */
   lcd_home();
   for(i=0;i<16;i++)lcd_data(disp_buf[i]);
   lcd_setcursor(0,2);
   for(i=16;i<32;i++)lcd_data(disp_buf[i]);
}

void menu_on_nav(int8_t dir) {
  if (dir==-1 && menu_idx>1) menu_idx--;
  else if (dir==1 && menu_idx+1<menu_len) menu_idx++;
  else BUZZ(200,30,10,4);
  redraw_mstimer=1;
}
void menu_on_action(char key) {
  tempbuf[0] = C_SET_MENU_ITEM;
  tempbuf[1] = key;
  tempbuf[2] = menu_udata;
  tempbuf[3] = menu_idx;
  rs485_message_send(menu_owner, 4, &tempbuf);
}

struct {
  uint8_t cmd;
  IRMP_DATA irmp_data;
} irmp_data;


int main (void) {            // (2)
          void (*bootloader)( void ) = 0x3C00;
   ledidx_t i,j;
/*
   uint8_t sensor_id[OW_ROMCODE_SIZE];
   uint8_t diff = OW_SEARCH_FIRST;
*/
   // initialize output PORTB
   DDRB  = 0xFF;
   PORTB = 0x00;

   //initialize_input(MOTION_DET_DDR);
   //MOTION_DET_PULLUP = 1;
   initialize_input(BELL_INPUT_DDR);

   initialize_output(BUZZER_DDR);

   // Initialize LCD Display
   initialize_output(LCD_RW_DDR);
   initialize_output(BACKLIT_DDR);
   LCD_RW_PORT = 0;
   BACKLIT_PORT = 1;

   _delay_ms(30); lcd_init();
   memset(disp_buf, '=', 32);
   
   lcd_string_P(PSTR("ham 0.5  "));
   lcd_setcursor(0,2);
   lcd_string_P(PSTR("Booting ...     "));

   lcd_command(LCD_SET_CGADR | (0 << 3));
   lcd_data(0b00000001);
   lcd_data(0b00000011);
   lcd_data(0b00000101);
   lcd_data(0b00001001);
   lcd_data(0b00000101);
   lcd_data(0b00000011);
   lcd_data(0b00000001);
   lcd_data(0);
   lcd_data(0b00001000);
   lcd_data(0b00001100);
   lcd_data(0b00001010);
   lcd_data(0b00001001);
   lcd_data(0b00001010);
   lcd_data(0b00001100);
   lcd_data(0b00001000);
   lcd_data(0);
   lcd_data(0b00000001);
   lcd_data(0b00000011);
   lcd_data(0b00000111);
   lcd_data(0b00001111);
   lcd_data(0b00000111);
   lcd_data(0b00000011);
   lcd_data(0b00000001);
   lcd_data(0);
   lcd_data(0b00001000);
   lcd_data(0b00001100);
   lcd_data(0b00001110);
   lcd_data(0b00001111);
   lcd_data(0b00001110);
   lcd_data(0b00001100);
   lcd_data(0b00001000);
   lcd_data(0);
/*
   initialize_output(RS485_DE_DDR);
   RS485_DE_PORT = 0;*/
   uart_init();
   //softuart_init();

   // Enable Interrupts
   sei();
   
   memcpy_P(&tempbuf, PSTR("\375doorctl"), 8);  //0375 = 0xfd
   rs485_message_send(0xff, 8, (uint8_t*)&tempbuf);



   lcd_setcursor(0,2);
   lcd_string_P(PSTR("Init LED        "));
   PORTB = 0xef;  //bootup 6

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

   // initialize buzzer timer
   TCCR1A = (1<<COM1B0);
   TIMSK |= (1<<OCIE1B);  // enable interrupt on output compare

   // Initialize millisecond timer
   TCCR2 = (1<<WGM21) | (1<<CS22);  //clear on compare, clock=f_osc/64
   OCR2 = 125; //f_osc=8000000,  8000000/64/125 = 1000Hz --> interrupt per millisec
   TIMSK |= (1<<OCIE2);  // enable interrupt on output compare
   keyspressed = 0;


   // Initialize IR timer
   // Interrupt at 12500 Hz, f_osc=8000000, prescale=1/64, compare=10
   TCCR0 = (1<<WGM01)|(1<<CS01)|(1<<CS00);
   TIMSK |= (1<<OCIE0);  // enable interrupt on output compare
   OCR0 = 10;



/*     ...ausgelötet
   ow_reset();
   DS18X20_find_sensor(&diff, &sensor_id[0]);
   if (diff == OW_PRESENCE_ERR) {
     lcd_setcursor(0,2);
     lcd_string_P(PSTR("No sensor found "));
     _delay_ms(1000);
   }else if (diff == OW_DATA_ERR){
     lcd_setcursor(0,2);
     lcd_string_P(PSTR("Data error "));
     _delay_ms(1000);
   }else  {
        onewire_mstimer = 1000;
        myflags.onewire_read_meas = 0;
   }*/

   PORTB = 0x00;  //bootup d
   lcd_setcursor(0,2);
   lcd_string_P(PSTR("Boot complete  "));
   _delay_ms(500);

   // Enter main loop
   redraw_mstimer = 1;
   backlit_stimer=7;


   while(1) {
    // read from softuart
    /*if (softuart_kbhit()) {
      recvbyte = softuart_getchar();
      receiveRS485();
    }*/

    if (recvidx == -3) { // incoming message in recvpkg
      //PIE1bits.RCIE = 0;
      //RCSTAbits.CREN = 0;
      if (recvpkg.st.length & (PF_ACK|PF_ERROR)) {
        //handle replies...
        //for now: ignore them
      } else {
        i = 0;
        switch (recvpkg.st.command) { //command
        case C_SET_OUTPUT: //set output
          if (recvpkg.st.data[1] == 0xff) j = 1;
          else if (recvpkg.st.data[1] == 0x00) j = 0;
          else { rs485_error(); break; }

          if (recvpkg.st.data[0] == 0x01) {
            LED_RED2_PORT = j;
            rs485_ack(0xAA);
          } else if (recvpkg.st.data[0] == 0x02) {
            LED_RED3_PORT = j;
            rs485_ack(0xAA);
          } else if (recvpkg.st.data[0] == 0x03) {
            LED_GREEN1_PORT = j;
            rs485_ack(0xAA);
          } else if (recvpkg.st.data[0] == 0x0e) {
            
            if (j == 1)
              TCCR1B = (1<<CS11)|(1<<CS10) | (1<<WGM12);
            else
              TCCR1B = 0;

            rs485_ack(0xAA);
          } else if (recvpkg.st.data[0] == 0x0f) {
            BACKLIT_PORT = j;
            rs485_ack(0xAA);
          } else if (recvpkg.st.data[0] == 0x10) {
            RELAY_PORT = j;
            if(j)relay_stimer = 10;
            rs485_ack(0xAA);
          } else {
            rs485_error();
          }
          break;

        case C_BUZZER:
            BUZZP(recvpkg.st.data[0], recvpkg.st.data[1],recvpkg.st.data[2],recvpkg.st.data[3],recvpkg.st.data[4]);
            rs485_ack(0xaa);
            break;

        case C_GET_TEMPERATURE: //get temperature
           tempbuf[0] = C_GET_TEMPERATURE;
           DS18X20_format_from_decicelsius( decicelsius, &tempbuf[1], 7 );
           rs485_message_send(recvpkg.st.fromaddr, 8|PF_ACK, (uint8_t*)tempbuf);
           break;

        case C_SET_DISPLAY: //set lcd display first row
           memcpy(disp_buf, recvpkg.st.data, 32);
           rs485_ack(0xaa);
           break;

        case C_SET_MENU: //set lcd display menu options
           myflags.in_menu = recvpkg.st.data[0];
           menu_udata = recvpkg.st.data[1];
           menu_idx = recvpkg.st.data[2];
           menu_len = 0;
           menu_owner = recvpkg.st.fromaddr;
           memset(menu_buf, 0, sizeof(menu_buf));
           rs485_ack(0xaa);
           break;

        case C_SET_MENU_ITEM: //set lcd display menu item text
           if (recvpkg.st.fromaddr!=menu_owner) {rs485_error(); break;}
           if (recvpkg.st.data[0]>MENU_MAX_ITEMS){rs485_error(); break;}

           memcpy(&menu_buf[recvpkg.st.data[0]], &recvpkg.st.data[1], MENU_TXT_LEN);
           if (recvpkg.st.data[0]>=menu_len && recvpkg.st.data[1])
              menu_len=recvpkg.st.data[0]+1;
           if (recvpkg.st.data[0]<menu_len && !recvpkg.st.data[1])
              menu_len=recvpkg.st.data[0];

           if (recvpkg.st.data[0] == menu_idx) redraw_mstimer=1; //if current item - redraw screen now
           rs485_ack(0xaa);
           break;

        case C_BMP_WRITE_RANGE: //set bmp rgb range
          j = 1;
          i = recvpkg.st.data[0]*3;
          while (j<recvpkg.st.length && i<STRIPE_LENGTH) {
            ws2801_buf[i] = recvpkg.st.data[j++];
            i++;
          }
          redraw_mstimer=1;
          rs485_ack(0xaa);
          break;

        case C_BMP_RANGE_FILL_CONST: //fill bmp const color range
        case C_BMP_RANGE_ADD_CONST: //dim/lighten bmp range
          i = recvpkg.st.data[0]*3;
          j=recvpkg.st.data[1]*3;
          if (recvpkg.st.data[0] >= STRIPE_LENGTH || recvpkg.st.data[1] >= STRIPE_LENGTH || recvpkg.st.data[1] < recvpkg.st.data[0]) {
            rs485_error(); break;
          }
          while(i<=j) {
            if (recvpkg.st.command == C_BMP_RANGE_FILL_CONST) {
              ws2801_buf[i] = recvpkg.st.data[2]; i++;
              ws2801_buf[i] = recvpkg.st.data[3]; i++;
              ws2801_buf[i] = recvpkg.st.data[4]; i++;
            } else {
              ws2801_buf[i] += (int8_t)recvpkg.st.data[2]; i++;
              ws2801_buf[i] += (int8_t)recvpkg.st.data[3]; i++;
              ws2801_buf[i] += (int8_t)recvpkg.st.data[4]; i++;
            }
          }
          rs485_ack(0xAA);
          break;
/*
        case C_BMP_TO_STRIPE_SINGLE: //update ws2812 from bitmap
          ws2801_draw_buffer(&ws2801_buf);
          rs485_ack(0xAA);
          break;*/
/*
        case C_SET_BAUD_RATE: //change baud rate
          test = recvpkg.st.data[0]<<8 | recvpkg.st.data[1];
          test32 = F_OSC;
          i = 0; j = 0;
          while(test32 != 0 && i!=255) { test32 -= test; j++; if (j==16) {i++; j=0;} }
          if (i==255) {rs485_error(); break; } //ging nicht auf
          //dbg(100);
          //i = F_OSC/test/16;
          //i = recvpkg.st.data[0];
          //dbg(300);
          rs485_ack(i);
          //dbg(500);
          RCSTAbits.SPEN = 0; // enable serial port
          RCSTAbits.CREN = 0;
          SPBRG = i;
          RCSTAbits.SPEN = 1; // enable serial port
          TXSTAbits.SYNC = 0; // async mode
          RCSTAbits.CREN = 1;
          break;*/
        case C_PING: // echo request
          //  LED_YELLOW_PORT=0;
          rs485_message_send(recvpkg.st.fromaddr, recvpkg.st.length | PF_ACK, &recvpkg.st.command);
          break;
        case C_SET_TIME: //set system time
          t_hour = recvpkg.st.data[0];
          t_min = recvpkg.st.data[1];
          t_sec = recvpkg.st.data[2];
          rs485_ack(0xAA);
          break;
        case C_REBOOT_TO_BOOTLOADER: // reboot to bootloader
          rs485_ack(0xBB);
          bootloader();
          break;
        default:
          if (!recvflags.broadcast) rs485_error();
          break;
        }
      }
      recvidx = -2; //start listening again
      //PIE1bits.RCIE = 1;
      //RCSTAbits.CREN = 1;
    }




     if (relay_stimer == 1) {
       PORTB &= ~(1<<PB4);
       PORTB &= ~((1<<PB5)|(1<<PB6)|(1<<PB7));
       relay_stimer = 0;
     }
     if (ws2801_scroll_speed != 0 && uart_mode != 0x01) {
       if (ws2801_scroll_timer == ws2801_scroll_speed) {
         ws2801_scroll_timer = 0;
         ws2801_scroll();
         ws2801_draw = 1;
       }
       ws2801_scroll_timer+=1;
     }
     /*
     if (onewire_mstimer == 1) {
      LED_GREEN4_PORT=1;
        if (myflags.onewire_read_meas) {
           if ( DS18X20_read_decicelsius( &sensor_id[0], &decicelsius) == DS18X20_OK ) {
           }
           myflags.onewire_read_meas = 0;
           onewire_mstimer = 15000;
        } else {
            if ( DS18X20_start_meas( DS18X20_POWER_PARASITE, NULL ) == DS18X20_OK) {
               myflags.onewire_read_meas = 1;
            }
            onewire_mstimer = DS18B20_TCONV_12BIT + 1;
        }
        LED_GREEN4_PORT=0;
     }
     */
     if (redraw_mstimer == 1) {
       // redraw led stripe
       ws2801_draw_buffer(&ws2801_buf[0]);
       ws2812_sendarray(&ws2801_buf[STRIPE_LENGTH*3]);

       if (disp_notice[0]) {
          if (disp_notice_scroll_mstimer==1) {
            disp_notice_scroll++;
            if (disp_notice_scroll+8>strlen(disp_notice)) disp_notice_scroll=0;
            disp_notice_scroll_mstimer=50;
          }
          if (disp_notice_stimer==1) {
            disp_notice[0] = 0;
            disp_notice_stimer = 0;
          }
          display_homescreen();
       } else if (myflags.in_menu) {
          // redraw LCD
          lcd_home();
          lcd_data(0x12);lcd_data(' ');
          for(i=0;i<12;i++)lcd_data(menu_buf[0][i]);
          lcd_data(' ');lcd_data(0x13);

          lcd_setcursor(0,2);
          if (menu_idx>1) lcd_data(0x02); //arrow left
                else lcd_data(0x00);
          for(i=0;i<14;i++)lcd_data(menu_buf[menu_idx][i]);
          if (menu_idx+1<menu_len) lcd_data(0x03);
                else lcd_data(0x01); //arrow right

       } else {
          display_homescreen();
       }

       redraw_mstimer = 251;
     }
     if (backlit_stimer == 1) {
       BACKLIT_PORT = 0; backlit_stimer = 0;
     }

     // motion detector
/*
     if (MOTION_DET_PIN) {
       if (!myflags.motion_det_edge) {
         tempbuf[0] = C_ON_INPUT; tempbuf[1] = 'M';
         rs485_message_send(0xff, 2, (uint8_t*)tempbuf);
         myflags.motion_det_edge = 1;
       }
       LED_GREEN5_PORT = 1;
         keypress_backlight();
     } else {
        myflags.motion_det_edge = 0;
        LED_GREEN5_PORT = 0;
     }*/

     // infrared remote control
     if (irmp_get_data (&irmp_data.irmp_data))
     {
        keypress_backlight();
        disp_notice_show(3);
        sprintf_P(disp_notice, PSTR("IR: %02x %04x %04x"), irmp_data.irmp_data.protocol
          , irmp_data.irmp_data.address
          , irmp_data.irmp_data.command);
        irmp_data.cmd = C_ON_INPUT;
        rs485_message_send(0xff, 7, (uint8_t*)&irmp_data);
     }

      if (keydownevent != 0) {
        if (keydownevent == KEY_OK) {
          keypress_backlight();
          if (disp_notice[0]) {
            disp_notice_dismiss();
          } else if (myflags.in_menu) {
            menu_on_action('A');
          } else {
          }
          
        }
        if (keydownevent == KEY_PREV) {
          keypress_backlight();
          if (disp_notice[0]) {
            disp_notice_dismiss();
          } else if (myflags.in_menu) {
            menu_on_nav(-1);
          } else {
          }
          
        }
        if (keydownevent == KEY_NEXT) {
          keypress_backlight();
          if (disp_notice[0]) {
            disp_notice_dismiss();
          } else if (myflags.in_menu) {
            menu_on_nav(1);
          } else {
          }
          
        }
        if (keydownevent == KEY_UP) {
          keypress_backlight();
          if (disp_notice[0]) {
            disp_notice_dismiss();
          } else if (myflags.in_menu) {
            menu_on_action('D');
          } else {
          }
          
        }
        if (keydownevent == KEY_CANCEL) {
          keypress_backlight();
          if (disp_notice[0]) {
            disp_notice_dismiss();
          } else if (myflags.in_menu) {
            myflags.in_menu = 0;
          } else {
            RELAY_PORT = 1;
            relay_stimer = 4;
          }
          
        }
        if (keydownevent == KEY_DOORBELL) {
          keypress_backlight();
          bell_mstimer = 0;
        }

        BUZZ(250,-5,10,1);
        for (i=0;i<8;i++) {
          if(keydownevent&(1<<i)) {
            tempbuf[0] = C_ON_INPUT; tempbuf[1] = i;
            rs485_message_send(0xff, 2, (uint8_t*)tempbuf);
          }
        }
        keydownevent = 0;
      }
      if (keyupevent != 0) {
        if (keyupevent == KEY_DOORBELL) {
          tempbuf[0] = C_ON_INPUT; tempbuf[1] = 42; tempbuf[2] = bell_mstimer>>8; tempbuf[3] = bell_mstimer&0xff;
          rs485_message_send(0xff, 4, (uint8_t*)tempbuf);
        }
        
        BUZZ(30,1,10,1);
        for (i=0;i<8;i++) {
          if(keyupevent&(1<<i)) {
            tempbuf[0] = C_ON_INPUT; tempbuf[1] = 128+i;
            rs485_message_send(0xff, 2, (uint8_t*)tempbuf);
          }
        }
        keyupevent = 0;
      }

   }
   
   return 0;
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
  ws2812_sendarray(&ws2801_buf[STRIPE_LENGTH*3]);
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

void ws2801_repeat_buffer(ledidx_t startIndex) {
  for(int i = startIndex; i != 3*STRIPE_LENGTH; i++) {
    ws2801_buf[i] = ws2801_buf[i-startIndex];
  }
}

void ws2801_scroll(void) {
  int tmpR = ws2801_buf[0], tmpG = ws2801_buf[1], tmpB = ws2801_buf[2];
  for(ledidx_t i = 0; i != STRIPE_LENGTH*3-3; i+=3) {
    ws2801_buf[i] = ws2801_buf[i+3];
    ws2801_buf[i+1] = ws2801_buf[i+4];
    ws2801_buf[i+2] = ws2801_buf[i+5];
  }
  ws2801_buf[(STRIPE_LENGTH-1)*3] = tmpR;
  ws2801_buf[(STRIPE_LENGTH-1)*3+1] = tmpG;
  ws2801_buf[(STRIPE_LENGTH-1)*3+2] = tmpB;

}

