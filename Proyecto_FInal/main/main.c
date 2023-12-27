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
#include "i2c_config.h"
#include "esp_event_base.h"
#include "freertos/queue.h"
#include "vl53l5cx_plugin_motion_indicator.h"


static const char *TAG = "main";

VL53L5CX_Configuration config;
uint8_t status, isAlive, isReady, i;
VL53L5CX_ResultsData results;
int numPersonas = 0;

QueueHandle_t Event_Queue;
esp_err_t status;


esp_err_t i2c_master_init(void)
{
    int i2c_port = 0;
    i2c_config_t i2c_config = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = 21,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_io_num = 22,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = VL53L5CX_MAX_CLK_SPEED,
    };
    i2c_param_config(i2c_port, &i2c_config);
    return i2c_driver_install(i2c_port, i2c_config.mode, 0, 0, 0);

}


void person_Enter(void){
	numPersonas++;
	ESP_LOGI(TAG, "Una persona ha entrado");
	ESP_LOGI(TAG, "Hay %d personas\n", numPersonas);
}

void person_Exit(void){
	numPersonas--;
	ESP_LOGI(TAG, "Una persona ha salido");
	ESP_LOGI(TAG, "Hay %d personas\n", numPersonas);
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
eSystemState Listening_State_Wifi_Person_Enter_Event(void){
    personEnter();
    return Listening_State;
}

eSystemState Listening_State_Wifi_Person_Exit_Event(void){
    personExit();
    return Listening_State;
}

eSystemState Listening_State_Wifi_HighSleep_Enter_Event(void){
	highSleepEnter();
	return HighSleep_State;
}

eSystemState Listening_State_Wifi_DeepSleep_Enter_Event(void){
	deepSleepEnter();
	return DeepSleep_State;
}

//HighSleep_State

eSystemState HighSleep_State_Wifi_HighSleep_Exit_Event(void){
	highSleepExit();
	return Listening_State;
}

eSystemState HighSleep_State_Wifi_DeepSleep_Enter_Event(void){
	deepSleepEnter();
	return DeepSleep_State;
}

//DeepSleep_State

eSystemState DeepSleep_State_Wifi_DeepSleep_Exit_Event(void){
	deepSleepExit();
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

static void wifi_highSleep_enter_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
	eSystemEvent eNewEvent = HighSleep_Enter_Event;
	xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void wifi_highSleep_exit_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
	eSystemEvent eNewEvent = HighSleep_Exit_Event;
	xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void wifi_deepSleep_enter_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
	eSystemEvent eNewEvent = DeepSleep_Enter_Event;
	xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void wifi_deepSleep_exit_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
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
                    eNextState = Listening_State_Wifi_Person_Enter_Event();
                }
				else if(eNewEvent == Person_Exit_Event){
					eNextState = Listening_State_Wifi_Person_Exit_Event();
				}
				else if(eNewEvent == HighSleep_Enter_Event){
					eNextState = Listening_State_Wifi_HighSleep_Enter_Event();
				}
				else if(eNewEvent == DeepSleep_Enter_Event){
					eNextState = Listening_State_Wifi_DeepSleep_Enter_Event();
				}
				break;
			case HighSleep_State:
				if(eNewEvent == HighSleep_Exit_Event){
					eNextState = HighSleep_State_Wifi_HighSleep_Exit_Event();
				}
				else if(eNewEvent == DeepSleep_Enter_Event){
					eNextState = HighSleep_State_Wifi_DeepSleep_Enter_Event();
				}
				break;
			case DeepSleep_State:
				if(eNewEvent == DeepSleep_Exit_Event){
					eNextState = DeepSleep_State_Wifi_DeepSleep_Exit_Event();
				}
				break;
		}
	}		

	*/

	

	ESP_ERROR_CHECK(i2c_master_init());

	uint8_t 				status, loop, isAlive, isReady, i;
    	VL53L5CX_Configuration 	Dev;			/* Sensor configuration */
    	VL53L5CX_Motion_Configuration 	motion_config;	/* Motion configuration*/
    	VL53L5CX_ResultsData 	Results;		/* Results data from VL53L5CX */
	config.platform.address = VL53L5CX_DEFAULT_I2C_ADDRESS;

	tatus = vl53l5cx_is_alive(&Dev, &isAlive);
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

	status = vl53l5cx_motion_indicator_init(&Dev, &motion_config, VL53L5CX_RESOLUTION_4X4);
	    if(status)
	    {
	        printf("Motion indicator init failed with status : %u\n", status);
	        return;
	    }
	status = vl53l5cx_motion_indicator_set_distance_motion(&Dev, &motion_config, 1000, 2000);
	    if(status)
	    {
	        printf("Motion indicator set distance motion failed with status : %u\n", status);
	        return;
	    }

	status = vl53l5cx_set_ranging_frequency_hz(&Dev, 2);
	status = vl53l5cx_start_ranging(&Dev);
	
	uint8_t loop = 0;
	while(loop < 10)
	{
		status = vl53l5cx_check_data_ready(&Dev, &isReady);

        if(isReady)
        {
            vl53l5cx_get_ranging_data(&Dev, &Results);

            /* As the sensor is set in 4x4 mode by default, we have a total
             * of 16 zones to print. For this example, only the data of first zone are
             * print */
            printf("Print data no : %3u\n", Dev.streamcount);
            for(i = 0; i < 16; i++)
            {
                printf("Zone : %3d, Motion power : %3lu\n",
                       i,
                       Results.motion_indicator.motion[motion_config.map_id[i]]);
            }
            printf("\n");
            loop++;
        }

        /* Wait a few ms to avoid too high polling (function in platform
         * file, not in API) */
        WaitMs(&(Dev.platform), 5);
    }

    status = vl53l5cx_stop_ranging(&Dev);
    printf("End of ULD demo\n");
}
