#include "people_counter.h"


ESP_EVENT_DEFINE_BASE(PEOPLE_COUNTER_EVENT);


void people_counter_init(){
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

    //uint32_t integration_time_ms;
    uint8_t status, isAlive;
    


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

    
}




void registerZone(int zone)
{
    if(zone != zoneList[listIndex - 1]){
        zoneList[listIndex] = zone;
        listIndex = (listIndex + 1) % 5;
        if(zone == 0){
            if(memcmp(zoneList, enterList, 5*sizeof(int)) == 0){
		        ESP_ERROR_CHECK(esp_event_post(PEOPLE_COUNTER_EVENT, PEOPLE_COUNTER_ENTER_EVENT, NULL, 0, portMAX_DELAY));
            }
            else if(memcmp(zoneList, exitList, 5*sizeof(int)) == 0){
                ESP_ERROR_CHECK(esp_event_post(PEOPLE_COUNTER_EVENT, PEOPLE_COUNTER_EXIT_EVENT, NULL, 0, portMAX_DELAY));
            }

            memcpy(zoneList, defaultList, 5*sizeof(int));
            listIndex = 1;
        }
    printf("zoneList: %d, %d, %d, %d, %d\n", zoneList[0], zoneList[1], zoneList[2], zoneList[3], zoneList[4]);
    }
    
}



void people_counter_start(void){

    /*********************************/
    /*         Ranging loop          */
    /*********************************/

    uint8_t status, isReady, i;


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
    
}





