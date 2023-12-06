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


static const char *TAG = "main";

VL53L5CX_Configuration config;
uint8_t status, isAlive, isReady, i;
VL53L5CX_ResultsData results;
int numPersonas = 0;

QueueHandle_t Event_Queue;
esp_err_t status;


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
	config.platform.address = VL53L5CX_DEFAULT_I2C_ADDRESS;

	// Verificar si el sensor está conectado
	status = vl53l5cx_is_alive(&config, &isAlive);
	if (!isAlive || status ) {
		printf("El sensor no está conectado. Por favor, verifica la conexión.\n");
		return;
	}
	// Inicializar el sensor con la configuración
	status = vl53l5cx_init(&config);
	if(status)
	{
		printf("Error al cargar el sensor VL53L5CX ULD\n");
		return;
	}

	// Comenzar a tomar medidas
	 vl53l5cx_start_ranging(&config);

	uint8_t loop = 0;
	while(loop < 10)
	{
		status = vl53l5cx_check_data_ready(&config, &isReady);
		if(isReady)
		{
			vl53l5cx_get_ranging_data(&config, &results);
			printf("Print data no : %3u\n", config.streamcount);
		}
		loop++;
		WaitMs(&(config.platform), 5);
	}
	printf("End of ULD demo\n");
}