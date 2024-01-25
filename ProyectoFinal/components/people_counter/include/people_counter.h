#ifndef PEOPLECOUNTER_H
#define PEOPLECOUNTER_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include <esp_err.h>
#include "vl53l5cx_api.h"
#include "vl53l5cx_plugin_motion_indicator.h"
#include <esp_log.h>
#include "esp_event.h"
#include "esp_task_wdt.h"

#define DISTANCETHRESHOLD 500

ESP_EVENT_DECLARE_BASE(PEOPLE_COUNTER_EVENT);

enum {
    PEOPLE_COUNTER_ENTER_EVENT,
    PEOPLE_COUNTER_EXIT_EVENT
};


//static const char *TAG = "people_counter";

static int zoneList[5] = {0, 0, 0, 0, 0};
static int defaultList[5] = {0, 0, 0, 0, 0};
static int enterList[5] = {0, 1, 3, 2, 0};
static int exitList[5] = {0, 2, 3, 1, 0};

static int listIndex = 1;


static VL53L5CX_Configuration 	Dev;			/* Sensor configuration */
static VL53L5CX_ResultsData 	Results;		/* Results data from VL53L5CX */

static esp_task_wdt_user_handle_t registerZone_twdt_user_handle;


void people_counter_init(void);        

void people_counter_start(void);

void registerZone(int zone);



#endif
