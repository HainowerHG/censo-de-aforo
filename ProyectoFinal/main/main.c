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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
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
#include "vl53l5cx_plugin_motion_indicator.h"´
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"



#define DISTANCETHRESHOLD 500

static const char *TAG = "main";
QueueHandle_t Event_Queue;

static int zoneList[5] = {0, 0, 0, 0, 0};
static int defaultList[5] = {0, 0, 0, 0, 0};
static int enterList[5] = {0, 1, 3, 2, 0};
static int exitList[5] = {0, 2, 3, 1, 0};

static int listIndex = 1;
static int timerCounter = 0;
static int hourCounter = 1;
static int32_t peopleCountSum = 0;
static int32_t peopleCounter = 0;
static char meanBuffer[20];
static char hourMeanKeyBuffer[15];
static char* nvs_read_data;



void person_Enter(void){
	peopleCounter++;
	ESP_LOGI(TAG, "Una persona ha entrado");
	ESP_LOGI(TAG, "Hay %" PRId32 " personas\n", peopleCounter);
}

void person_Exit(void){
	peopleCounter--;
	ESP_LOGI(TAG, "Una persona ha salido");
	ESP_LOGI(TAG, "Hay %" PRId32 " personas\n", peopleCounter);
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


void nvs_write_values_int32_t(char* key, int32_t value){
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");

    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle", esp_err_to_name(err));
    } else {
        printf("Done\n");
        
        printf("Updating %s in NVS ...", key);
        err = nvs_set_i32(my_handle, key, value);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);
    }

    printf("\n");
}



void nvs_write_values_str(char* key, char* value){
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");

    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle", esp_err_to_name(err));
    } else {
        printf("Done\n");
        
        printf("Updating %s in NVS ...", key);
        err = nvs_set_str(my_handle, key, value);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);
    }

    printf("\n");
}


void nvs_read_values(void){
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");

    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle", esp_err_to_name(err));
    } else {
       printf("Done\n");

        // Read
        printf("Reading data from NVS ... \n");
        err = nvs_get_i32(my_handle, "peopleCount", &peopleCounter);
        switch (err) {
            case ESP_OK:
                printf("People counter = %" PRIu32 "\n", peopleCounter);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("People counter is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        for(int i = 1; i <= 24; i++){
            char key[15];
            snprintf(key, sizeof(key), "peopleMean%d", i);
            size_t required_size;
            nvs_get_str(my_handle, key, NULL, &required_size);
            nvs_read_data = malloc(required_size);
            err =  nvs_get_str(my_handle, key, nvs_read_data, &required_size);
            switch (err) {
                case ESP_OK:
                    printf("%s = %s\n", key, nvs_read_data);
                    break;
                case ESP_ERR_NVS_NOT_FOUND:
                    printf("%s is not initialized yet!\n", key);
                    break;
                default :
                    printf("Error (%s) reading!\n", esp_err_to_name(err));
            }
        }

        nvs_close(my_handle);
    }

    printf("\n");
}


void registerZone(int zone)
{
    if(zone != zoneList[listIndex - 1]){
        zoneList[listIndex] = zone;
        listIndex = (listIndex + 1) % 5;
        if(zone == 0){
            if(memcmp(zoneList, enterList, 5*sizeof(int)) == 0){
                peopleCounter++;
                ESP_LOGI(TAG, "Entrada. Numero de Personas: %" PRId32 "", peopleCounter);
                nvs_write_values_int32_t("peopleCount", peopleCounter);
            }
            else if(memcmp(zoneList, exitList, 5*sizeof(int)) == 0){
                peopleCounter--;
                ESP_LOGI(TAG, "Salida. Numero de Personas: %" PRId32 "", peopleCounter);
                nvs_write_values_int32_t("peopleCount", peopleCounter);
            }

            memcpy(zoneList, defaultList, 5*sizeof(int));
            listIndex = 1;
        }
    printf("zoneList: %d, %d, %d, %d, %d\n", zoneList[0], zoneList[1], zoneList[2], zoneList[3], zoneList[4]);
    }
    
}

static void mean_periodic_timer_callback(void* arg)
{
    peopleCountSum += peopleCounter; 
    if(timerCounter == 5){
        timerCounter = 0;
        snprintf(meanBuffer, sizeof(meanBuffer), "%.2f" , (float)peopleCountSum/6);
        snprintf(hourMeanKeyBuffer, sizeof(hourMeanKeyBuffer), "peopleMean%d", hourCounter);
        nvs_write_values_str(hourMeanKeyBuffer, meanBuffer);
        ESP_LOGI(TAG, "Media de la hora %i: %s\n", hourCounter, meanBuffer);
        peopleCountSum = 0;
        hourCounter = (hourCounter % 24) + 1;
    }
    else
        timerCounter++;

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
    //ESP_ERROR_CHECK(nvs_flash_erase()); //Quitar comentario para borrar la memoria flash
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    peopleCounter = 0;
    //Cargamos el valor de NVS en peopleCounter
    nvs_read_values();


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
    uint32_t 				integration_time_ms;
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



   	 /*********************************/
	/*        Set some params        */
	/*********************************/

	/* Set resolution in 8x8. WARNING : As others settings depend to this
	 * one, it must be the first to use.
	 */
	
    /*
    status = vl53l5cx_set_resolution(&Dev, VL53L5CX_RESOLUTION_8X8);
	if(status)
	{
		printf("vl53l5cx_set_resolution failed, status %u\n", status);
		return;
	}
    */

	/* Set ranging frequency to 10Hz.
	 * Using 4x4, min frequency is 1Hz and max is 60Hz
	 * Using 8x8, min frequency is 1Hz and max is 15Hz
	 */
	status = vl53l5cx_set_ranging_frequency_hz(&Dev, 10);
	if(status)
	{
		printf("vl53l5cx_set_ranging_frequency_hz failed, status %u\n", status);
		return ;
	}

	/* Set target order to closest */
	status = vl53l5cx_set_target_order(&Dev, VL53L5CX_TARGET_ORDER_CLOSEST);
	if(status)
	{
		printf("vl53l5cx_set_target_order failed, status %u\n", status);
		return ;
	}

	status = vl53l5cx_set_ranging_mode(&Dev, VL53L5CX_RANGING_MODE_AUTONOMOUS);
	if(status)
	{
		printf("vl53l5cx_set_ranging_mode failed, status %u\n", status);
		return;
	}

	/* Using autonomous mode, the integration time can be updated (not possible
	 * using continuous) */
	status = vl53l5cx_set_integration_time_ms(&Dev, 20);


    const esp_timer_create_args_t mean_periodic_timer_args = {
            .callback = &mean_periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "mean_periodic_timer"
    };

    esp_timer_handle_t mean_periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&mean_periodic_timer_args, &mean_periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(mean_periodic_timer, 2000000)); //2s

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
