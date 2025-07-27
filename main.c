#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include <stdio.h>
#include <string.h>

#define NUM_COLUMNS 80
#define NUM_DATA_PINS 12
#define START_DELAY_US 9000
#define COLUMN_READ_DELAY_US 3400 //3400 was good

#define COLUMNS 80

#define BITMASK (0x00001BFF)


static repeating_timer_t sampler_tmr;
static volatile unsigned int col_ctr = 0;

static uint16_t sample[COLUMNS];

bool sampler_cb(repeating_timer_t *rt)
{
    sample[col_ctr++] = (uint16_t)(gpio_get_all() & BITMASK);
    return (col_ctr < COLUMNS);
}

long long int sampler_start_cb(alarm_id_t id, void *user_data)
{
    (void)id;
    (void)user_data;
    col_ctr = 0;
    sample[col_ctr++] = (uint16_t)(gpio_get_all() & BITMASK);
    add_repeating_timer_us(COLUMN_READ_DELAY_US, sampler_cb, NULL, &sampler_tmr);
    return 0;
}


bool card_inserted() {
    // Wait until all 12 pins go low (0) = blocking light (card inserted)
    uint16_t pin_state = gpio_get_all() & BITMASK;
    return pin_state != BITMASK;
}

bool card_finished() {
    // Wait until all 12 pins go low (0) = blocking light (card inserted)
    uint16_t pin_state = gpio_get_all() & BITMASK;
    return pin_state == BITMASK;
}

int main() {
    unsigned int i, j;
    stdio_init_all();
    sleep_ms(1000);

    // Initialize GPIOs 0 through 11 as inputs
    for (i = 0; i < NUM_DATA_PINS; ++i) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        gpio_pull_up(i);  // Enable pull-up
    }
    gpio_init(18);
    gpio_init(19);
    gpio_set_dir(18, GPIO_IN);
    gpio_set_dir(19, GPIO_IN);


    printf("Waiting for card...\n");


    while (1) {
        // Wait for card to be inserted (all pins 0)
        while (!card_inserted())
            ;
        add_alarm_in_us(START_DELAY_US, sampler_start_cb, NULL, true);

        /* Do nothing. Sampling happens in callbacks from timers */
        while(col_ctr < COLUMNS)
            sleep_ms(10);

        cancel_repeating_timer(&sampler_tmr);
        while(!card_finished())
            ;

#if 0
        for (i = 0; i < COLUMNS; i++) {
            printf("Col %d: %04X %0b\n", i, sample[i], sample[i]);
        }
#endif
        for (i = 0; i < COLUMNS; i++) {
            printf((sample[i] & (1u << 12u))?"*":"_");
        }
        printf("\n");
        for (i = 0; i < COLUMNS; i++) {
            printf((sample[i] & (1u << 11u))?"*":"_");
        }
        printf("\n");
        for (j = 0; j < 10; j++) {
            for (i = 0; i < COLUMNS; i++) {
                printf((sample[i] & (1u << j))?"*":"_");
            }
            printf("\n");
        }
        printf("\n\n");
        col_ctr = 0;
    }
    return 0;
}

