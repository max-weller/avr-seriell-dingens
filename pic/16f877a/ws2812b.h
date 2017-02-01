

// RC 2 -> output ws2812
#define WSOUT_PORT PORTCbits.RC2
#define WSOUT_TRIS TRISCbits.TRISC2

#define STRIPE_LENGTH 73

extern uint8_t bmp_r[STRIPE_LENGTH];
extern uint8_t bmp_g[STRIPE_LENGTH];
extern uint8_t bmp_b[STRIPE_LENGTH];

void ws2812b_constcolor(uint8_t red, uint8_t green, uint8_t blue, uint8_t count);

void ws2812b_fromarray(uint8_t count);

