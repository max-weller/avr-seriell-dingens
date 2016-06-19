


#define BAUD 9600
#include <util/setbaud.h>

void uart_init(void)
{
   UBRRH = UBRRH_VALUE;
   UBRRL = UBRRL_VALUE;
   /* evtl. verkuerzt falls Register aufeinanderfolgen (vgl. Datenblatt)
      UBRR = UBRR_VALUE;
   */
#if USE_2X
   /* U2X-Modus erforderlich */
   UCSRA |= (1 << U2X);
#else
   /* U2X-Modus nicht erforderlich */
   UCSRA &= ~(1 << U2X);
#endif

   // hier weitere Initialisierungen (TX und/oder RX aktivieren, Modus setzen
   UCSRB |= (1<<TXEN) | (1<<RXEN) | (1<<RXCIE);
}


