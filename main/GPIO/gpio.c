/*
 * gpio.c
 *
 *  Created on: Dec 15, 2023
 *      Author: odemki
 */

#include  "gpio.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

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
