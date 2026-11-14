#include <neorv32.h>

#define BAUD       1000000       /* must match APP_BAUD in the Python scripts */
#define DIGIT_BASE 0x44A00000UL
#define REG(off)   (*(volatile uint32_t *)(DIGIT_BASE + (off)))

#define AP_CTRL    0x000
#define AP_RETURN  0x010
#define IMG_BASE   0x400
#define N_WORDS    196          /* 784 bytes / 4 */

int main(void) {
    int first = 1;

    neorv32_uart0_setup(BAUD, 0, 0);
    neorv32_uart0_putc('R');

    while (1) {
        for (int n = 0; n < N_WORDS; n++) {
            uint32_t w = 0;
            for (int b = 0; b < 4; b++) {
                w |= ((uint32_t)(uint8_t)neorv32_uart0_getc()) << (8 * b);
            }
            if (first && n == 0) neorv32_uart0_putc('1');
            REG(IMG_BASE + 4 * n) = w;
            if (first && n == 0) neorv32_uart0_putc('2');
        }

        uint32_t t0 = (uint32_t)neorv32_cpu_get_cycle();
        REG(AP_CTRL) = 1;
        if (first) neorv32_uart0_putc('3');
        while (!(REG(AP_CTRL) & 0x2));
        uint32_t t1 = (uint32_t)neorv32_cpu_get_cycle();
        if (first) { neorv32_uart0_putc('4'); first = 0; }

        uint32_t digit  = REG(AP_RETURN);
        uint32_t cycles = t1 - t0;

        neorv32_uart0_putc((char)digit);
        for (int k = 0; k < 4; k++) {
            neorv32_uart0_putc((char)(cycles >> (8 * k)));
        }
    }
    return 0;
}
