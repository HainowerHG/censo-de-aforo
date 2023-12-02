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

VL53L5CX_Configuration config;
uint8_t status, isAlive, isReady, i;
VL53L5CX_ResultsData results;

void app_main(void)
{
	
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