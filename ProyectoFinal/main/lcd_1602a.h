#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"


static const char *TAGH = "lcd";

extern int RS_PIN;
extern int EN_PIN;
extern int D4_PIN;
extern int D5_PIN;
extern int D6_PIN;
extern int D7_PIN;



void init_lcd(int RS_PIN, int EN_PIN, int D4_PIN, int D5_PIN, int D6_PIN, int D7_PIN) {
    ESP_LOGI(TAGH, "Configuring Pins");
    gpio_reset_pin(RS_PIN);
    gpio_reset_pin(EN_PIN);
    gpio_reset_pin(D4_PIN);
    gpio_reset_pin(D5_PIN);
    gpio_reset_pin(D6_PIN);
    gpio_reset_pin(D7_PIN);

    ESP_LOGI(TAGH, "Configuring Pins Direction");
    gpio_set_direction(RS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D4_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D5_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D6_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D7_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAGH, "Configuring LCD");
    // set RS_PIN low for config mode
    gpio_set_level(RS_PIN, 0);
    // set lcd in 4 bit mode
    gpio_set_level(D4_PIN, 0);
    gpio_set_level(D5_PIN, 1);
    gpio_set_level(D6_PIN, 0);
    gpio_set_level(D7_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 0);

    // clear lcd
    gpio_set_level(D4_PIN, 0);
    gpio_set_level(D5_PIN, 0);
    gpio_set_level(D6_PIN, 0);
    gpio_set_level(D7_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 0);

    gpio_set_level(D4_PIN, 1);
    gpio_set_level(D5_PIN, 0);
    gpio_set_level(D6_PIN, 0);
    gpio_set_level(D7_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 0);

    // return home
    gpio_set_level(D4_PIN, 0);
    gpio_set_level(D5_PIN, 0);
    gpio_set_level(D6_PIN, 0);
    gpio_set_level(D7_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 0);

    gpio_set_level(D4_PIN, 0);
    gpio_set_level(D5_PIN, 1);
    gpio_set_level(D6_PIN, 0);
    gpio_set_level(D7_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 0);

    // turn lcd on, no cursor mode
    gpio_set_level(D4_PIN, 0);
    gpio_set_level(D5_PIN, 0);
    gpio_set_level(D6_PIN, 0);
    gpio_set_level(D7_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 0);

    gpio_set_level(D4_PIN, 0);
    gpio_set_level(D5_PIN, 0);
    gpio_set_level(D6_PIN, 1);
    gpio_set_level(D7_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(EN_PIN, 0);
}

void printToLcd(char c[]) {
    int length;
    int pinVal;
    int pins[4] = {D4_PIN, D5_PIN, D6_PIN, D7_PIN};
    int pin_counter = 3;
    
    length = strlen(c);
    
    // loop over string
    // length -1 to make sure we skip the termination char \0
    for( int i = 0; i <= length-1; i++ ) {
        int x = c[i];
        
        for( int i = 7; i >= 0; i-- ) {
            // get pin value according to bit
            pinVal = (( x >> i ) & 1 ? 1 : 0 );
            //ESP_LOGI(TAGH, "setting pin %d to %d", pins[pin_counter], pinVal);
            // set corresponding pin value to pin
            gpio_set_level(pins[pin_counter], pinVal);
            --pin_counter;

            // If we reached middle or end of 8 bit char
            // then set enable to high to send command to lcd
            // reset pin counter for next 4 bits
            if (i == 4 || i == 0) {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(EN_PIN, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(EN_PIN, 0);
                pin_counter = 3;
            }
        }
    }
}