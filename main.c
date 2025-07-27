#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include <stdio.h>
#include <string.h>

#define NUM_COLUMNS 80
#define NUM_DATA_PINS 12
#define DATA_PINS_MASK 0x0FFF  // Pins 0-11 mask
#define START_DELAY_US 9480
#define COLUMN_READ_DELAY_US 3400

#define COLUMNS 80

#define BITMASK (~0xFFFFF400)


static repeating_timer_t sampler_tmr;
static unsigned int col_ctr = 0;

static uint16_t sample[COLUMNS];

bool sampler_cb(repeating_timer_t *rt)
{
#if 0
    int i;
    uint32_t loc_sample[10];
    uint32_t sel_sample = 0;
    uint32_t count = 0;

    while(1) 
    {
        for (i = 0; i < 5; i++)
        {
            loc_sample[i] = gpio_get_all() & BITMASK;
        }
        sel_sample = loc_sample[0];
        for (i = 1; i < 5; i++) {
            if (loc_sample[i] == sel_sample) {
                count++;
            } else {
                sel_sample = loc_sample[i];
                count = 0;
            }
            if (count > 2) {
                sample[col_ctr++] = (uint16_t)(sel_sample & BITMASK);
                return true;
            }
        }
    }
#endif
    sample[col_ctr++] = (uint16_t)(gpio_get_all() & BITMASK);
    return 1;
}

bool sampler_start_cb(alarm_id_t id, void *user_data)
{
    (void)id;
    (void)user_data;
    col_ctr = 0;
    sample[col_ctr++] = gpio_get_all() & BITMASK;
    add_repeating_timer_us(COLUMN_READ_DELAY_US, sampler_cb, NULL, &sampler_tmr);
    return 0;
}


bool card_inserted() {
    // Wait until all 12 pins go low (0) = blocking light (card inserted)
    uint16_t pin_state = gpio_get_all() & BITMASK;
    return pin_state == 0;
}

int main() {
    int i;
    stdio_init_all();
    sleep_ms(2000);

    // Initialize GPIOs 0 through 11 as inputs
    for (i = 0; i < NUM_DATA_PINS; ++i) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        gpio_pull_up(i);  // Enable pull-up if needed
    }

    printf("Waiting for card...\n");

    while (1) {
        // Wait for card to be inserted (all pins 0)
        while (!card_inserted())
            ;
        add_alarm_in_us(START_DELAY_US, sampler_start_cb, NULL, true);
        while(col_ctr < COLUMNS)
            sleep_ms(10);
        while(card_inserted())
            ;
        for (i = 0; i < COLUMNS; i++) {
            printf("Col %d: %04X\n", i, sample[i]);
        }
    }
    return 0;
}

