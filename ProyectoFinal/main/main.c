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
#include "esp_event_base.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lcd_1602a.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_timer.h"
#include "people_counter.h"

int RS_PIN = 5;
int EN_PIN = 18;
int D4_PIN = 4;
int D5_PIN = 19;
int D6_PIN = 17;
int D7_PIN = 16;


static const char *TAG = "main";
QueueHandle_t Event_Queue;

const char *base_path = "/spiflash";
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
static const char *log_path = "/spiflash/log.txt";


static int32_t peopleCount = 0;
static int32_t peopleCountSum = 0;
static int timerCounter = 0;
static int hourCounter = 1;

static char meanBuffer[20];
static char hourMeanKeyBuffer[15];

static char* nvs_read_data;


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
        err = nvs_get_i32(my_handle, "peopleCount", &peopleCount);
        switch (err) {
            case ESP_OK:
                printf("People counter = %" PRIu32 "\n", peopleCount);
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



void person_Enter(void){
	peopleCount++;
	ESP_LOGI(TAG, "Una persona ha entrado");
	ESP_LOGI(TAG, "Hay %" PRId32 " personas\n", peopleCount);
    char cadena[50];  
    printf(cadena, "NUM PERSONAS:  %" PRId32 "", peopleCount);
    printToLcd(cadena);
    nvs_write_values_int32_t("peopleCount", peopleCount);
	
}

void person_Exit(void){
	peopleCount--;
	ESP_LOGI(TAG, "Una persona ha salido");
	ESP_LOGI(TAG, "Hay %" PRId32 " personas\n", peopleCount);
    nvs_write_values_int32_t("peopleCount", peopleCount);
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

static void people_counter_enter_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    eSystemEvent eNewEvent = Person_Enter_Event;
    xQueueSend(Event_Queue, &eNewEvent, portMAX_DELAY);
}

static void people_counter_exit_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
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





static void mean_periodic_timer_callback(void* arg)
{
    peopleCountSum += peopleCount; 
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


static void partition_write(void){
    //ESP_LOGI(TAG, "Opening file");
    printf("Opening file\n");
    FILE *f = fopen("/spiflash/hello.txt", "wb");
    if (f == NULL) {
        //ESP_LOGE(TAG, "Failed to open file for writing");
        printf("Failed to open file for writing\n");
        return;
    }
    fprintf(f, "written using ESP-IDF %s\n", esp_get_idf_version());
    fprintf(f, "Hola esto es un mensaje de prueba");
    fclose(f);
    ESP_LOGI(TAG, "File written");
}

static void partition_read(void){
    printf("Reading File\n");
    FILE *f = fopen("/spiflash/log.txt", "rb");
    if (f == NULL) {
       printf("Failed to open file for reading");
        return;
    }
    char line[128];
    /*fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);*/

    while (fgets(line, sizeof(line), f) != NULL) {
        // strip newline
        char *pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }
        //ESP_LOGI(TAG, "Read from file: '%s'", line);
        printf("Read from file: '%s'\n", line);
    }
    fclose(f);
}


static int log_print_manager(const char *fmt, va_list args){

    char log_level = fmt[7];

    if(log_level == 'E' || log_level == 'W'){
        FILE *log_file;

        //printf("Se escribe en el archivo %s\n", log_path);

        if (access(log_path, F_OK) == -1) {
            // El archivo no existe, crearlo
            log_file = fopen(log_path, "wb");
            if (log_file == NULL) {
                printf("Error al crear el archivo\n");
                return -1;
            }
            printf("Se creó el archivo %s\n", log_path);
        } else {
        
            log_file = fopen(log_path, "a");
            if (log_file == NULL) {
                printf("Error al abrir el archivo %s\n", log_path);
                return -1;
            }
        }
        int ret = vfprintf(log_file, fmt, args);
        fclose(log_file);
        //printf("Se ha escrito en el archivo%s\n", log_path);
        return ret;
    }
    else{
        return vprintf(fmt, args);
    }
}


void app_main(void)
{


    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount("/spiflash", "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }

    esp_log_level_set("*", ESP_LOG_INFO);

    esp_log_set_vprintf(&log_print_manager);

    /*
    ESP_LOGE(TAG, "Log de Error");
    ESP_LOGW(TAG, "Log de Warning");
    ESP_LOGI(TAG, "Log de Informacion");
    ESP_LOGD(TAG, "Log de Debug");
    ESP_LOGV(TAG, "Log de Verbose");

    */

    partition_read();


    //ESP_ERROR_CHECK(nvs_flash_erase()); //Quitar comentario para borrar la memoria flash

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    peopleCount = 0;
    //Cargamos el valor de NVS en peopleCounter
    nvs_read_values();

    init_lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

    // Set RS high for write mode
    // This is kept here in case more configs
    //    need to be sent to lcd before print
    gpio_set_level(RS_PIN, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);


    const esp_timer_create_args_t mean_periodic_timer_args = {
            .callback = &mean_periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "mean_periodic_timer"
    };

    esp_timer_handle_t mean_periodic_timer;

    ESP_ERROR_CHECK(esp_timer_create(&mean_periodic_timer_args, &mean_periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(mean_periodic_timer, 2000000)); //2s

    

    //partition_write();
    //partition_read();


    //Descomentar cuando implementemos eventos
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_event_handler_instance_register(PEOPLE_COUNTER_EVENT, PEOPLE_COUNTER_ENTER_EVENT, people_counter_enter_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(PEOPLE_COUNTER_EVENT, PEOPLE_COUNTER_EXIT_EVENT, people_counter_exit_handler, NULL, NULL));

	
	Event_Queue = xQueueCreate(20, sizeof(eSystemEvent));

    eSystemState eNextState = Listening_State;
    eSystemEvent eNewEvent;

    TaskHandle_t xHandle;

    people_counter_init();

    // xTaskCreate(&people_counter_start, "people_counter_task", 4096, NULL, 5, &xHandle); // este me genera errore por el bocle infinito y la perición del core hhg
     xTaskCreatePinnedToCore(&people_counter_start, "people_counter_task", 4096, NULL, 5, &xHandle,1);
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

    ESP_LOGI(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK( esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle));

    ESP_LOGI(TAG, "Done");
}
