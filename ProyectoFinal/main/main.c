/*******************************************************************************
* Copyright (c) 2020, STMicroelectronics - All Rights Reserved
*
* This file is part of the VL53L5CX Ultra Lite Driver and is dual licensed,
* either 'STMicroelectronics Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, the VL53L5CX Ultra Lite Driver may be distributed under the
* terms of 'BSD 3-clause "New" or "Revised" License', in which case the
* following provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
*******************************************************************************/

/***********************************/
/*   VL53L5CX ULD basic example    */
/***********************************/
/*
* This example is the most basic. It initializes the VL53L5CX ULD, and starts
* a ranging to capture 10 frames.
*
* By default, ULD is configured to have the following settings :
* - Resolution 4x4
* - Ranging period 1Hz
*
* In this example, we also suppose that the number of target per zone is
* set to 1 , and all output are enabled (see file platform.h).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "vl53l5cx_api.h"
#include "esp_event_base.h"
#include "freertos/queue.h"
#include "vl53l5cx_plugin_motion_indicator.h"


#define DISTANCETHRESHOLD 500

static const char *TAG = "main";
QueueHandle_t Event_Queue;

static int zoneList[5] = {0, 0, 0, 0, 0};
static int defaultList[5] = {0, 0, 0, 0, 0};
static int enterList[5] = {0, 1, 3, 2, 0};
static int exitList[5] = {0, 2, 3, 1, 0};

static int listIndex = 1;
static int numPeople = 0;


void registerZone(int zone)
{
    if(zone != zoneList[listIndex - 1]){
        zoneList[listIndex] = zone;
        listIndex = (listIndex + 1) % 5;
        if(zone == 0){
            if(memcmp(zoneList, enterList, 5*sizeof(int)) == 0){
                numPeople++;
                ESP_LOGI(TAG, "Entrada. Numero de Personas: %d", numPeople);
            }
            else if(memcmp(zoneList, exitList, 5*sizeof(int)) == 0){
                numPeople--;
                ESP_LOGI(TAG, "Salida. Numero de Personas: %d", numPeople);
            }

            memcpy(zoneList, defaultList, 5*sizeof(int));
            listIndex = 1;
        }
    printf("zoneList: %d, %d, %d, %d, %d\n", zoneList[0], zoneList[1], zoneList[2], zoneList[3], zoneList[4]);
    }
    
}

void person_Enter(void){
	numPeople++;
	ESP_LOGI(TAG, "Una persona ha entrado");
	ESP_LOGI(TAG, "Hay %d personas\n", numPeople);
}

void person_Exit(void){
	numPeople--;
	ESP_LOGI(TAG, "Una persona ha salido");
	ESP_LOGI(TAG, "Hay %d personas\n", numPeople);
}

void highSleep_Enter(void){
	ESP_LOGI(TAG, "Entra en HighSleep");
}

void highSleep_Exit(void){
	ESP_LOGI(TAG, "Sale de HighSleep");
}

void deepSleep_Enter(void){
	ESP_LOGI(TAG, "Entra en DeepSleep");
}

void deepSleep_Exit(void){
	ESP_LOGI(TAG, "Sale de DeepSleep");
}

typedef enum{
    Listening_State,
	HighSleep_State,
    DeepSleep_State
} eSystemState;

typedef enum{
	Person_Enter_Event,
    Person_Exit_Event,
	HighSleep_Enter_Event,
	HighSleep_Exit_Event,
	DeepSleep_Enter_Event,
	DeepSleep_Exit_Event
} eSystemEvent;


//Listening_State
eSystemState Listening_State_Person_Enter_Event(void){
    person_Enter();
    return Listening_State;
}

eSystemState Listening_State_Person_Exit_Event(void){
    person_Exit();
    return Listening_State;
}

eSystemState Listening_State_HighSleep_Enter_Event(void){
	highSleep_Enter();
	return HighSleep_State;
}

eSystemState Listening_State_DeepSleep_Enter_Event(void){
	deepSleep_Enter();
	return DeepSleep_State;
}

//HighSleep_State

eSystemState HighSleep_State_HighSleep_Exit_Event(void){
	highSleep_Exit();
	return Listening_State;
}

eSystemState HighSleep_State_DeepSleep_Enter_Event(void){
	deepSleep_Enter();
	return DeepSleep_State;
}

//DeepSleep_State

eSystemState DeepSleep_State_DeepSleep_Exit_Event(void){
	deepSleep_Exit();
	return Listening_State;
}

static void sensor_enter_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    eSystemEvent eNewEvent = Person_Enter_Event;
    xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void sensor_exit_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
	eSystemEvent eNewEvent = Person_Exit_Event;
	xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void highSleep_enter_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
	eSystemEvent eNewEvent = HighSleep_Enter_Event;
	xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void highSleep_exit_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
	eSystemEvent eNewEvent = HighSleep_Exit_Event;
	xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void deepSleep_enter_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
	eSystemEvent eNewEvent = DeepSleep_Enter_Event;
	xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void deepSleep_exit_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
	eSystemEvent eNewEvent = DeepSleep_Exit_Event;
	xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}


void app_main(void)
{

    /*
	//Descomentar cuando implementemos eventos
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_event_handler_instance_register(SENSOR_EVENT, SENSOR_EVENT_ENTER, sensor_enter_handler, NULL, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(SENSOR_EVENT, SENSOR_EVENT_EXIT, sensor_exit_handler, NULL, NULL));

	
	Event_Queue = xQueueCreate(20, sizeof(eSystemEvent));

    eSystemState eNextState = Listening_State;
    eSystemEvent eNewEvent;

 	while(1){
        
        xQueueReceive(Event_Queue, &eNewEvent, portMAX_DELAY);

        switch (eNextState)
        {
			case Listening_State:
				if(eNewEvent == Person_Enter_Event){
                    eNextState = Listening_State_Person_Enter_Event();
                }
				else if(eNewEvent == Person_Exit_Event){
					eNextState = Listening_State_Person_Exit_Event();
				}
				else if(eNewEvent == HighSleep_Enter_Event){
					eNextState = Listening_State_HighSleep_Enter_Event();
				}
				else if(eNewEvent == DeepSleep_Enter_Event){
					eNextState = Listening_State_DeepSleep_Enter_Event();
				}
				break;
			case HighSleep_State:
				if(eNewEvent == HighSleep_Exit_Event){
					eNextState = HighSleep_State_HighSleep_Exit_Event();
				}
				else if(eNewEvent == DeepSleep_Enter_Event){
					eNextState = HighSleep_State_DeepSleep_Enter_Event();
				}
				break;
			case DeepSleep_State:
				if(eNewEvent == DeepSleep_Exit_Event){
					eNextState = DeepSleep_State_DeepSleep_Exit_Event();
				}
				break;
		}
	}		

	*/



    //Define the i2c bus configuration
    i2c_port_t i2c_port = I2C_NUM_1;
    i2c_config_t i2c_config = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = 26,
            .scl_io_num = 27,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = VL53L5CX_MAX_CLK_SPEED,
    };

    i2c_param_config(i2c_port, &i2c_config);
    i2c_driver_install(i2c_port, i2c_config.mode, 0, 0, 0);

    /*********************************/
    /*   VL53L5CX ranging variables  */
    /*********************************/

    uint8_t 				status, loop, isAlive, isReady, i;
    VL53L5CX_Configuration 	Dev;			/* Sensor configuration */
    VL53L5CX_ResultsData 	Results;		/* Results data from VL53L5CX */


    /*********************************/
    /*      Customer platform        */
    /*********************************/

    /* Fill the platform structure with customer's implementation. For this
    * example, only the I2C address is used.
    */
    Dev.platform.address = VL53L5CX_DEFAULT_I2C_ADDRESS;
    Dev.platform.port = i2c_port;

    /* (Optional) Reset sensor toggling PINs (see platform, not in API) */
    //Reset_Sensor(&(Dev.platform));

    /* (Optional) Set a new I2C address if the wanted address is different
    * from the default one (filled with 0x20 for this example).
    */
    //status = vl53l5cx_set_i2c_address(&Dev, 0x20);


    /*********************************/
    /*   Power on sensor and init    */
    /*********************************/

    /* (Optional) Check if there is a VL53L5CX sensor connected */
    status = vl53l5cx_is_alive(&Dev, &isAlive);
    if(!isAlive || status)
    {
        printf("VL53L5CX not detected at requested address\n");
        return;
    }

    /* (Mandatory) Init VL53L5CX sensor */
    status = vl53l5cx_init(&Dev);
    if(status)
    {
        printf("VL53L5CX ULD Loading failed\n");
        return;
    }

    printf("VL53L5CX ULD ready ! (Version : %s)\n",
           VL53L5CX_API_REVISION);



    //Activamos el modo 8x8
    //status = vl53l5cx_set_resolution(&Dev, VL53L5CX_RESOLUTION_8X8);


    /*********************************/
    /*         Ranging loop          */
    /*********************************/

    status = vl53l5cx_start_ranging(&Dev);

   int zone1, zone2, result;
    while(1)
    {
        zone1 = 0;
        zone2 = 0;
        result = 0;
        /* Use polling function to know when a new measurement is ready.
         * Another way can be to wait for HW interrupt raised on PIN A1
         * (INT) when a new measurement is ready */

        status = vl53l5cx_check_data_ready(&Dev, &isReady);

        if(isReady)
        {
            vl53l5cx_get_ranging_data(&Dev, &Results);

            /* As the sensor is set in 4x4 mode by default, we have a total
             * of 16 zones to print. For this example, only the data of first zone are
             * print */
            for(i = 0; i < 8; i++) //for(i = 0; i < 32; i++)
                if(Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*i] < DISTANCETHRESHOLD)
                    zone1 = 1;
            for(i = 8; i < 16; i++) //for(i = 0; i < 32; i++)
                if(Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*i] < DISTANCETHRESHOLD)
                    zone2 = 2;
            
            result = zone1 + zone2;
            registerZone(result);
        }

        /* Wait a few ms to avoid too high polling (function in platform
         * file, not in API) */
        WaitMs(&(Dev.platform), 5);
    }

    status = vl53l5cx_stop_ranging(&Dev);
    printf("End of ULD demo\n");
}