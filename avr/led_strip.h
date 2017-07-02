
#define WS2801_OUT_PORT_DDR DDRA
#define WS2801_OUT_PORT PORTA
#define WS2801_DATA_PIN PA2
#define WS2801_CLK_PIN PA1

#define STRIPE_LENGTH   93      //Anzahl der LED Elemente
#define WS2801_LATCH_DELAY  500    //500µs minimal für Latch 
#define WS2801_BIT_PERIOD   20      //Takte verzögerung zw 2 Signale
#define WS2801_R_DEF        128      //Default Rotanteil
#define WS2801_G_DEF        128      //Default Grünanteil
#define WS2801_B_DEF        128      //Default Blauanteil
#define rot   0              //Konstante für Arrayzugriff Rot
#define  gruen 1              //Konstante für Arrayzugriff Grün
#define  blau  2              //Konstante für Arrayzugriff Blau

#define WS2801_BIT_DELAY (WS2801_BIT_PERIOD/2)

typedef uint16_t ledidx_t;

void ws2801_draw_buffer(uint8_t *RGB_data_send);
void ws2801_draw_all(uint8_t _ws2801_current_red,uint8_t _ws2801_current_green,uint8_t _ws2801_current_blue);
void ws2801_reset(void);
void ws2801_init(void);
void _ws2801_send_byte(uint8_t byte);

