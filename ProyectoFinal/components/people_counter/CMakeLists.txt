idf_component_register(SRCS "people_counter.c"
                      INCLUDE_DIRS "include"
                       REQUIRES driver
                       REQUIRES esp_event
                       PRIV_REQUIRES rjrp44__vl53l5cx)