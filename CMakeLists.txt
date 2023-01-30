idf_component_register(SRCS "mupeWeb.c"
                    INCLUDE_DIRS "include" 
                    REQUIRES "esp_http_server"  "esp_event"  "esp_wifi" "nvs_flash"
                     EMBED_TXTFILES "favicon.ico" )
