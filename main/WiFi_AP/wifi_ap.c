/*
 * wifi_ap.c
 *
 *  Created on: Dec 15, 2023
 *      Author: odemki
 */

#include "main.h"


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
/*
	README
	Дівайс виступає у вигляді Accsses Point. Тобто він не підключається до WiFi роутера, а створює власну
	точку доступу.

	1. Підключитися до точки доступу.
	2. Ввести пароль доступу  password:123qwerty
	3. Дівайс видасть свій IP: 192.168.4.2
	4. З браузера зайти на http://192.168.4.1/ledoff
	5. ПОявиться кнопка, якою можна включати і виключати LED на ESP32

*/

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN


static const char *TAG_WIFI = "webserver";

//--------------------------------------------------------------------------------------------------------
/* An HTTP GET handler */
static esp_err_t ledOFF_handler(httpd_req_t *req)
{
	esp_err_t error;
	ESP_LOGI(TAG_WIFI, "LED Turned OFF");
	const char *response = (const char *)req->user_ctx;
	error = httpd_resp_send(req, response, strlen(response));
	if(error != ESP_OK)
	{
		ESP_LOGI(TAG_WIFI, "ERROR %d while sending response", error);
	}
	else
	{
		set_led_state(0);
		ESP_LOGI(TAG_WIFI, "Response send Seccessfuly");
	}
	return error;
}
//--------------------------------------------------------------------------------------------------------
static const httpd_uri_t ledoff = {
    .uri       = "/ledoff",
    .method    = HTTP_GET,
    .handler   = ledOFF_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
.button {\
  border: none;\
  color: white;\
  padding: 15px 32px;\
  text-align: center;\
  text-decoration: none;\
  display: inline-block;\
  font-size: 16px;\
  margin: 4px 2px;\
  cursor: pointer;\
}\
\
.button1 {background-color: #04AA6D;} /* Green */\
</style>\
</head>\
<body>\
\
<h1>ESP32 WEBSERVER</h1>\
<p>Toggle the onboard LED</p>\
<h3> LED STATE : OFF</h3>\
\
<button class=\"button button1\" onclick= \"window.location.href='/ledon'\"> LED ON</button>\
\
</body>\
</html>"
};

//--------------------------------------------------------------------------------------------------------
/* An HTTP GET handler */
static esp_err_t ledON_handler(httpd_req_t *req)
{
	esp_err_t error;
	ESP_LOGI(TAG_WIFI, "LED Turned ON");
	const char *response = (const char *)req->user_ctx;
	error = httpd_resp_send(req, response, strlen(response));
	if(error != ESP_OK)
	{
		ESP_LOGI(TAG_WIFI, "ERROR %d while sending response", error);
	}
	else
	{
		set_led_state(1);
		ESP_LOGI(TAG_WIFI, "Response send Seccessfuly");
	}
	return error;
}
//--------------------------------------------------------------------------------------------------------
static const httpd_uri_t ledon = {
    .uri       = "/ledon",
    .method    = HTTP_GET,
    .handler   = ledON_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
.button {\
  border: none;\
  color: white;\
  padding: 15px 32px;\
  text-align: center;\
  text-decoration: none;\
  display: inline-block;\
  font-size: 16px;\
  margin: 4px 2px;\
  cursor: pointer;\
}\
\
.button1 {background-color: #000000;} /* Green */\
</style>\
</head>\
<body>\
\
<h1>ESP32 WEBSERVER</h1>\
<p>Toggle the onboard LED</p>\
<h3> LED STATE : ON</h3>\
\
<button class=\"button button1\" onclick= \"window.location.href='/ledoff'\"> LED OFF</button>\
\
</body>\
</html>"
};



//--------------------------------------------------------------------------------------------------------


/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}
//--------------------------------------------------------------------------------------------------------
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG_WIFI, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG_WIFI, "Registering URI handlers");
        httpd_register_uri_handler(server, &ledoff);
        httpd_register_uri_handler(server, &ledon);
        return server;
    }

    ESP_LOGI(TAG_WIFI, "Error starting server!");
    return NULL;
}
//--------------------------------------------------------------------------------------------------------
static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}
//--------------------------------------------------------------------------------------------------------
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG_WIFI, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG_WIFI, "Failed to stop http server");
        }
    }
}
//--------------------------------------------------------------------------------------------------------
void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG_WIFI, "Starting webserver");
        *server = start_webserver();
    }
}
//--------------------------------------------------------------------------------------------------------
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG_WIFI, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG_WIFI, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}
//--------------------------------------------------------------------------------------------------------
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

//--------------------------------------------------------------------------------------------------------




