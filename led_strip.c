#include <avr/io.h>          // (1)
#include "led_strip.h"
//delay.h einbinden für verzögerungen
#include <util/delay.h>

// Variablen
// Farbwerte 
volatile uint8_t _ws2801_current_red = 0;
volatile uint8_t _ws2801_current_green = 0;
volatile uint8_t _ws2801_current_blue = 0;
//Standart Routinen

void _ws2801_send_byte(uint8_t byte)
{
    uint8_t outbyte;
    for(uint8_t i=7; i!=255; i--)
    {
        outbyte = (byte>>i) & 0x01; //Daten zerlegen bitweise

        // Set the data pin to given value, and clk pin to 0
        if(outbyte)
        {
            WS2801_OUT_PORT |= (outbyte << WS2801_DATA_PIN);
        }
        else
        {
            WS2801_OUT_PORT &= ~(outbyte << WS2801_DATA_PIN);
        }
        WS2801_OUT_PORT &= ~(1 <<WS2801_CLK_PIN);
        _delay_us(WS2801_BIT_DELAY);

        // Keep the data pin, and set clk pin to 1 (strobe)
        WS2801_OUT_PORT |= 1 << WS2801_CLK_PIN;
        _delay_us(WS2801_BIT_DELAY);

        // Zero both clk and data pins
        WS2801_OUT_PORT &= ~((1 << WS2801_DATA_PIN) | (1 << WS2801_CLK_PIN));
    }
}

void ws2801_draw_all(uint8_t _ws2801_current_red,uint8_t _ws2801_current_green,uint8_t _ws2801_current_blue)
{
    for(uint8_t i=0; i <= WS2801_STRIP_LEN; i++)
    {
        _ws2801_send_byte(_ws2801_current_red);
        _ws2801_send_byte(_ws2801_current_green);
        _ws2801_send_byte(_ws2801_current_blue);
    }
    // Daten übertragen, jetzt auf den latch warten
    _delay_us(WS2801_LATCH_DELAY);
}



void ws2801_init()
{
// PIN Daten vorbereiten (I/O und Datenrichtungsregister)
    WS2801_OUT_PORT_DDR |= (1 << WS2801_CLK_PIN) | (1 << WS2801_DATA_PIN);
    WS2801_OUT_PORT &= ~((1 << WS2801_DATA_PIN) | (1 << WS2801_CLK_PIN));
}


void ws2801_reset()
{
    ws2801_draw_all(WS2801_R_DEF,WS2801_G_DEF,WS2801_B_DEF);
}


// Alle LEDS mit vordefinierten Werten laden und latchen
// Daten übertragen
void ws2801_draw_buffer(unsigned char *RGB_data_send)
{
    
  
  for(uint16_t i=0; i<=WS2801_STRIP_LEN*3; i++)
    {
        _ws2801_send_byte(RGB_data_send[i]);

    }

    // Daten übertragen, jetzt auf den latch warten
    _delay_us(WS2801_LATCH_DELAY);
  
}


