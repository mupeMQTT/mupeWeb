#ifndef MUPEWEB_H
#define MUPEWEB_H

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include "esp_http_server.h"
//#include "nvs_flash.h"
#include <esp_wifi.h>

void addHttpd_uri(httpd_uri_t *httpd_uri, const char *linkTxt);
void mupeWebInit();

#endif
