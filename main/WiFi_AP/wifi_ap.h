/*
 * wifi_ap.h
 *
 *  Created on: Dec 15, 2023
 *      Author: odemki
 */

#ifndef MAIN_WIFI_AP_WIFI_AP_H_
#define MAIN_WIFI_AP_WIFI_AP_H_

#include "main.h"
#include "esp_event_base.h"

void wifi_init_softap(void);
void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);
#endif /* MAIN_WIFI_AP_WIFI_AP_H_ */
