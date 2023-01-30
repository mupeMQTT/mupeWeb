#include "mupeWeb.h"
#define STARTS_WITH(string_to_check, prefix) (strncmp(string_to_check, prefix, (strlen(prefix))))
static const char *TAG = "example";

static httpd_handle_t server = NULL;

typedef struct Httpd_uri_list Httpd_uri_list;
struct Httpd_uri_list {
	httpd_uri_t httpd_uri;
	const char *linkTxt;
	Httpd_uri_list *next;
};

Httpd_uri_list *httpd_uri_listP = NULL;

void addHttpd_uri(httpd_uri_t *httpd_uri, const char *linkTxt) {
	Httpd_uri_list *httpd_uri_listL = NULL;
	httpd_uri_listL = httpd_uri_listP;

	Httpd_uri_list *list = NULL;
	list = malloc(sizeof(Httpd_uri_list));
	list->httpd_uri = *httpd_uri;
	list->linkTxt = linkTxt;
	list->next = NULL;
	if (httpd_uri_listL == NULL) {
		httpd_uri_listP = list;
		return;
	}
	if (httpd_uri_listL->next == NULL) {
		httpd_uri_listL->next = list;
		return;
	}

	while (httpd_uri_listL->next != NULL) {
		httpd_uri_listL = httpd_uri_listL->next;
	}
	httpd_uri_listL->next = list;

}



esp_err_t default_get_handler(httpd_req_t *req) {
	extern const unsigned char mqtt_index_start[] asm("_binary_index_html_start");
	extern const unsigned char mqtt_index_end[] asm("_binary_index_html_end");
	const size_t mqtt_index_size = (mqtt_index_end - mqtt_index_start);
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char*) mqtt_index_start, mqtt_index_size);
	return ESP_OK;

}

/* Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico.
 * This can be overridden by uploading file with same name */

esp_err_t favicon_get_handler(httpd_req_t *req) {
	extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
	extern const unsigned char favicon_ico_end[] asm("_binary_favicon_ico_end");
	const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char*) favicon_ico_start, favicon_ico_size);
	return ESP_OK;
}

esp_err_t root_get_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "HTTPS req %s ", req->uri);
	if (STARTS_WITH(req->uri, "/favicon.ico") == 0) {
		return favicon_get_handler(req);
	}
	return default_get_handler(req);
}

httpd_uri_t favicon_t = { .uri = "/favicon.ico", .method = HTTP_GET, .handler =
		favicon_get_handler };

void mupeStop_webserver(httpd_handle_t server) {
	// Stop the httpd server
	httpd_stop(server);
}

esp_err_t file_not_foud_handler(httpd_req_t *req, httpd_err_code_t error) {
	httpd_resp_set_type(req, "text/html");
	char buffer[500];
	strcpy(buffer, "<!DOCTYPE html><html><body><h1>HTML Links</h1>");
	Httpd_uri_list *httpd_uri_listL = httpd_uri_listP;
	while (httpd_uri_listL != NULL) {
		if (httpd_uri_listL->httpd_uri.method == HTTP_GET) {
			if (httpd_uri_listL->httpd_uri.uri[strlen(
					httpd_uri_listL->httpd_uri.uri) - 1] == '*') {
				strcat(buffer, "<p><a href=");
				strcat(buffer, httpd_uri_listL->httpd_uri.uri);
				buffer[strlen(buffer) - 1] = 0;
				strcat(buffer, ">");
				strcat(buffer, httpd_uri_listL->linkTxt);
				strcat(buffer, "</a></p>");
			}
		}
		httpd_uri_listL = httpd_uri_listL->next;
	}
	strcat(buffer, "</body></html>");
	httpd_resp_send(req, buffer, strlen(buffer));
	return ESP_OK;

}

httpd_handle_t mupeStart_webserver(void) {
	httpd_handle_t server = NULL;

	// Start the httpd server
	ESP_LOGI(TAG, "Starting server");

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.uri_match_fn = httpd_uri_match_wildcard;

	esp_err_t ret = httpd_start(&server, &config);
	if (ESP_OK != ret) {
		ESP_LOGI(TAG, "Error starting server!");
		return NULL;
	}
	ESP_LOGI(TAG, "Registering URI handlers");
	Httpd_uri_list *httpd_uri_listL = httpd_uri_listP;
	while (httpd_uri_listL != NULL) {
		httpd_register_uri_handler(server, &(httpd_uri_listL->httpd_uri));
		ESP_LOGI(TAG, "Registering URI handlers %s",
				httpd_uri_listL->httpd_uri.uri);
		httpd_uri_listL = httpd_uri_listL->next;
	}
	httpd_register_err_handler(server, HTTPD_404_NOT_FOUND,
			&file_not_foud_handler);
	return server;
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) {
	httpd_handle_t *server = (httpd_handle_t*) arg;
	if (*server) {
		mupeStop_webserver(*server);
		*server = NULL;
	}
}

static void connect_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) {
	httpd_handle_t *server = (httpd_handle_t*) arg;
	if (*server == NULL) {
		*server = mupeStart_webserver();
	}
}
char *faviconTxt = "hhhhh";
void mupeWebInit() {
	addHttpd_uri(&favicon_t, faviconTxt);
	server = mupeStart_webserver();

	ESP_ERROR_CHECK(
			esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
					&connect_handler, &server));
	ESP_ERROR_CHECK(
			esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED,
					&connect_handler, &server));

	ESP_ERROR_CHECK(
			esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
					&disconnect_handler, &server));
	ESP_ERROR_CHECK(
			esp_event_handler_register(WIFI_EVENT,
					WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler,
					&server));

}
