#ifndef LCD1602A_H
#define LCD1602A_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

extern int RS_PIN;
extern int EN_PIN;
extern int D4_PIN;
extern int D5_PIN;
extern int D6_PIN;
extern int D7_PIN;

void init_lcd(int RS_PIN, int EN_PIN, int D4_PIN, int D5_PIN, int D6_PIN, int D7_PIN);
//void clear_lcd(int EN_PIN, int D4_PIN, int D5_PIN, int D6_PIN, int D7_PIN);
void printToLcd(char c[]);


#endif // LCD1602A_H
