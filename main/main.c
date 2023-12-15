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

static const char *TAG_WIFI = "webserver";





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

				sprintf(str_h ,"%d_", qLogGPSData_t.time.hour+2);
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






