/* NMEA Parser example, that decode data stream from GPS receiver

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



#include "main.h"



#define TIME_ZONE (+8)   //Beijing Time
#define YEAR_BASE (2000) //date in GPS starts from 2000



//////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	uint8_t hour;      /*!< Hour */
	uint8_t minute;    /*!< Minute */
	uint8_t second;    /*!< Second */
	uint16_t thousand; /*!< Thousand */
} my_gps_time_t;

typedef struct {
    uint8_t day;   /*!< Day (start from 1) */
    uint8_t month; /*!< Month (start from 1) */
    uint16_t year; /*!< Year (start from 2000) */
} my_gps_date_t;

typedef struct
{
	float latitude;
	float longitude;
	float altitude;
	float speed;
	my_gps_time_t time;
	my_gps_date_t date;
	uint8_t sats_in_view;

}qLogGPSData;

QueueHandle_t gps_data_queue = NULL;

//////////////////////////////////////////////////////////////////////////////////




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

#define BLINK_GPIO CONFIG_BLINK_GPIO
////////////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG_WIFI = "webserver";

//--------------------------------------------------------------------------------------------------------
void set_led_state(uint8_t state)
{
	static const char *TAG = "GPIO";

	if((state > 1) || (state < 0))
	{
		ESP_LOGE(TAG, "wrong parameter");
	}
	else
	{
		gpio_set_level(BLINK_GPIO, state);
	}
}
//--------------------------------------------------------------------------------------------------------
void gpio_init(void)
{
	gpio_reset_pin(BLINK_GPIO);
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

	for(int i = 0; i < 10; i++)
	{
		set_led_state(i%2);
		vTaskDelay(30/portTICK_PERIOD_MS);
	}
}
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
static void connect_handler(void* arg, esp_event_base_t event_base,
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









////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////


//nmea_parser_config_t config;
//nmea_parser_handle_t nmea_hdl;

/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief GPS Event Handler
 *
 * @param event_handler_arg handler specific arguments
 * @param event_base event base, here is fixed to ESP_NMEA_EVENT
 * @param event_id event id
 * @param event_data event specific arguments
 */
static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	static const char *TAG = "GPS: ";

	qLogGPSData qLogGPSData_t;

    gps_t *gps = NULL;

    switch (event_id) {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        // Fill in queue
        qLogGPSData_t.latitude = gps->latitude;
        qLogGPSData_t.longitude = gps->longitude;
        qLogGPSData_t.altitude = gps->altitude;
        qLogGPSData_t.speed = gps->speed;

        qLogGPSData_t.time.hour = gps->tim.hour;
        qLogGPSData_t.time.minute = gps->tim.minute;
        qLogGPSData_t.time.second = gps->tim.second;

        qLogGPSData_t.date.year = gps->date.year + YEAR_BASE;
        qLogGPSData_t.date.month = gps->date.month;
        qLogGPSData_t.date.day = gps->date.day;

        xQueueSendToBack(gps_data_queue, &qLogGPSData_t, 0);

        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}

// --------------------------------------------------------------------------------------------------
void turn_on_gps(void)
{
	nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();	    /* NMEA parser configuration */
	nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);		/* init NMEA parser library */
	nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);		/* register event handler for NMEA parser library */
}
// --------------------------------------------------------------------------------------------------
void turn_off_gps(void)
{
//	nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();	    /* NMEA parser configuration */
//	nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);		/* init NMEA parser library */
//	nmea_parser_remove_handler(nmea_hdl, gps_event_handler);		/* unregister event handler */
//	nmea_parser_deinit(nmea_hdl);									//    /* deinit NMEA parser library */
}
// --------------------------------------------------------------------------------------------------

static void gps_log_task(void *arg)
{
	static const char *TAG = "FILE";
	ESP_LOGI(TAG, "Initializing SPIFFS");

	BaseType_t qStatus = false;
	qLogGPSData qLogGPSData_t;

	esp_vfs_spiffs_conf_t conf = {
			.base_path = "/spiffs",
			.partition_label = NULL,
		    .max_files = 5,
		    .format_if_mount_failed = true
	};

	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
		{
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND)
		{
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else
		{
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else
	{
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}


	char gps_buf_data[200] = {0,};


	while(1)
	{
		memset(gps_buf_data, 0, sizeof(gps_buf_data));

		qStatus = xQueueReceive(gps_data_queue, &qLogGPSData_t, 1000/portTICK_PERIOD_MS);
		if(qStatus == pdPASS)
		{
			// Add all data to one string for save into file
			snprintf(gps_buf_data, sizeof(gps_buf_data),
					"%d:%d:%d %d:%d:%d latitude:%.06f°N longitude:%.06f°E altitude:%.02fm speed:%.02fm\n"
					,
					qLogGPSData_t.date.year, qLogGPSData_t.date.month, qLogGPSData_t.date.day,
					qLogGPSData_t.time.hour+2, qLogGPSData_t.time.minute, qLogGPSData_t.time.second,
					qLogGPSData_t.latitude,qLogGPSData_t.longitude,qLogGPSData_t.altitude, qLogGPSData_t.speed

			);

			ESP_LOGI(TAG, "%s", gps_buf_data);

			static char name_of_file[30] = {0,};
			static uint8_t create_file = 1;

			if(create_file == 1)			// Make name file for logging GPS data
			{
				char path_to_log_file[] = "/spiffs/";
				char str_1[] = "file";
				char end[] = ".txt";
				char str_h[5] = {0,};
				char str_m[5] = {0,};
				char str_s[5] = {0,};
				strcpy(name_of_file, path_to_log_file);
				strcat(name_of_file, str_1);

				sprintf(str_h ,"%d_", qLogGPSData_t.time.hour);
				sprintf(str_m ,"%d_",qLogGPSData_t.time.minute);
				sprintf(str_s ,"%d",qLogGPSData_t.time.second);

				strcat(name_of_file, str_h);
				strcat(name_of_file, str_m);
				strcat(name_of_file, str_s);
				strcat(name_of_file, end);

				ESP_LOGI(TAG, "File name: %s", name_of_file);

				create_file = 0;
			}


			FILE* f;
			//ESP_LOGI(TAG, "Opening file");

			f = fopen(name_of_file, "a");				// Add GPS data into file
			if (f == NULL)
			{
				ESP_LOGE(TAG, "Failed to open file for writing");
				return;
			}

			// Записати лог даних в файл
			fprintf(f, gps_buf_data);
			ESP_LOGI(TAG, "Saving on file");
			fclose(f);
		}
	}
}
// --------------------------------------------------------------------------------------------------
void init_and_run_WiFi_AP(void)
{
	static httpd_handle_t server = NULL;
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG_WIFI, "ESP_WIFI_MODE_AP");
	wifi_init_softap();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));
}


// --------------------------------------------------------------------------------------------------
void app_main(void)
{
	vTaskDelay(2000/portTICK_PERIOD_MS);

	gpio_init();
	init_and_run_WiFi_AP();


	turn_on_gps();

	gps_data_queue = xQueueCreate(5, sizeof(qLogGPSData));

	xTaskCreate(gps_log_task, "gps_log_task", 4048, NULL, configMAX_PRIORITIES, NULL);


	while(1)
	{

		vTaskDelay(1000/portTICK_PERIOD_MS);
	}

}
// --------------------------------------------------------------------------------------------------






