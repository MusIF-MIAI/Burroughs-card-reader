#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include <stdio.h>
#include <string.h>

#define NUM_COLUMNS 80
#define NUM_DATA_PINS 13

#define COLUMNS 80
#define CARD_CLOCKS 684
#define TOO_MANY_CLOCKS 700

#define BITMASK (0x00001BFF)

#define START_COUNT 4
#define COLUMN_READ_COUNT 8

#define GPIO_ODOMETER 18
#define GPIO_CARD_DETECT 19


static repeating_timer_t sampler_tmr;
static volatile unsigned int col_ctr = 0;
#define MAX_CARDS 1000

static uint16_t sample[MAX_CARDS][COLUMNS];
static uint32_t card_time[MAX_CARDS];
static volatile int n_cards = 0;
static volatile uint32_t jiffies = 0;

static volatile unsigned int sample_clock_count = 0;

static volatile unsigned int card_inserted = 0;


void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == GPIO_CARD_DETECT) {
        if ((events & GPIO_IRQ_EDGE_FALL) && (card_inserted == 0)) {
            card_time[n_cards] = jiffies;
            col_ctr = 0;
            sample_clock_count = START_COUNT;
            card_inserted = 1;
            return;
        }
        if ((events & GPIO_IRQ_EDGE_RISE) && (card_inserted == 1)) {
            card_time[n_cards++] = jiffies;
            card_inserted = 0;
            jiffies = 0;
        }
    } else if (gpio == GPIO_ODOMETER) {
        jiffies++;
        if (!card_inserted)
            return;
        if (jiffies == TOO_MANY_CLOCKS) {
            printf("Warning: possible MISS at position %d\n", n_cards + 1); 
        }
        if (col_ctr >= COLUMNS) {
            sample_clock_count = 0;
        }
        if (sample_clock_count == 0)
            return;
        if (--sample_clock_count == 0) {
            sample[n_cards][col_ctr++] = (uint16_t)(gpio_get_all() & BITMASK);
            sample_clock_count = COLUMN_READ_COUNT;
        }
    }
}

int main() {
    unsigned int i, j, card;
    stdio_init_all();
    sleep_ms(2000);

    // Initialize GPIOs 0 through 12 as inputs, except 10
    for (i = 0; i < NUM_DATA_PINS; ++i) {
        if (i == 10)
            continue;
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        gpio_pull_up(i);  // Enable pull-up
    }
    gpio_init(GPIO_ODOMETER);
    gpio_set_dir(GPIO_ODOMETER, GPIO_IN);
    gpio_pull_down(GPIO_ODOMETER);
    gpio_init(GPIO_CARD_DETECT);
    gpio_set_dir(GPIO_CARD_DETECT, GPIO_IN);
    gpio_pull_down(GPIO_CARD_DETECT);

    gpio_set_irq_enabled_with_callback(GPIO_ODOMETER, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(GPIO_CARD_DETECT, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    printf("Waiting for card...\n");

    while (1) {
        char c;
        // Wait for card to be inserted (all pins 0)
        while (!card_inserted) {
            c = getchar_timeout_us(0);
            if (c == ' ') {
                for (card = 0; card < n_cards; card++) {
                    printf("Card n. %d\n", card + 1);
                    for (i = 0; i < COLUMNS; i++) {
                        printf((sample[card][i] & (1u << 12u))?"*":"_");
                    }
                    printf("\n");
                    for (i = 0; i < COLUMNS; i++) {
                        printf((sample[card][i] & (1u << 11u))?"*":"_");
                    }
                    printf("\n");
                    for (j = 0; j < 10; j++) {
                        for (i = 0; i < COLUMNS; i++) {
                            printf((sample[card][i] & (1u << j))?"*":"_");
                        }
                        printf("\n");
                    }
                    printf("\n\n");
                }
                printf("Total cards: %d\n", n_cards + 1);
                printf("\n\n");
            }
            if (c == 'a' || c == 'A') {
                for (card = 0; card < n_cards; card++) {
                    printf("Card n. %d\n", card + 1);
                    for (i = 0; i < COLUMNS; i++) {
                        printf("%04X ", sample[card][i]);
                    }
                    printf("\n");
                }
                printf("Total cards: %d\n", n_cards + 1);
                printf("\n\n");
            }
        }
        asm volatile("wfi");
    }
    return 0;
}

