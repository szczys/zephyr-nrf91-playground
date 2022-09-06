/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>
LOG_MODULE_REGISTER(basic_spi, LOG_LEVEL_DBG);

#include "golioth_app.h"

#define SPI_OP	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/* https://stackoverflow.com/a/69152884 */
#include <drivers/gpio.h>
#include <drivers/spi.h>

/*
 * Validate data received from MCP3201
 */
int validate_adc_data(uint8_t buf_data[4], uint16_t *adc_validated) {
	if (buf_data[0] & 1<<5) { return -ENOTSUP; }	/* Missing NULL bit */
	uint16_t data_msb = 0;
	uint16_t data_lsb = 0;
	data_msb = buf_data[0] & 0x1F;
	data_msb |= (data_msb<<7) | (buf_data[1]>>1);
	for (uint8_t i=0; i<12; i++) {
		bool bit_set = false;
		if (i < 2) {
			if (buf_data[1] & (1<<(1-i))) { bit_set = true; }
		}
		else if (i < 10) {
			if (buf_data[2] & (1<<(2+7-i))) { bit_set = true; }
		}
		else {
			if (buf_data[3] & (1<<(10+7-i))) { bit_set = true; }
		}

		if (bit_set) { data_lsb |= (1<<i); }
	}

					/* d[1].1 d[1].0 d[2].7 d[2].6 ... d[3].7 */
	if (data_msb != data_lsb) {
		LOG_DBG("data_msb=0x%04x data_lsb=0x%04x", data_msb, data_lsb);
		return -ENOTSUP;
	}
	
	*adc_validated = data_msb;	
	return 0;
}

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
	LOG_INF("Started SPI demo");
	int ret;

	golioth_app_init();

	if (!device_is_ready(led.port)) {
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}
	const struct spi_dt_spec mcp3201_dev =
		SPI_DT_SPEC_GET(DT_NODELABEL(mcp3201), SPI_OP, 0);

	LOG_INF("spi_cs.bus = %p", mcp3201_dev.bus);
	LOG_INF("spi_cs.config.cs->gpio.port = %p", mcp3201_dev.config.cs->gpio.port);
	LOG_INF("spi_cs.config.cs->gpio.pin = %u", mcp3201_dev.config.cs->gpio.pin);
	
	uint8_t my_buffer[4] = {0};
	struct spi_buf my_spi_buffer[1];
	my_spi_buffer[0].buf = my_buffer;
	my_spi_buffer[0].len = 4;
	const struct spi_buf_set rx_buff = { my_spi_buffer, 1 };
	uint16_t valid_adc_data = 0;

	while (1) {
		ret = spi_read_dt(&mcp3201_dev, &rx_buff);
		if (ret) { LOG_INF("spi_read status: %d", ret); }
		LOG_INF("Received 4 bytes: %d %d %d %d",
					my_buffer[0],my_buffer[1],my_buffer[2], my_buffer[3]
					);
		ret = validate_adc_data(my_buffer, &valid_adc_data);
		if (ret == 0) { LOG_INF("Got ADC value: 0x%04x", valid_adc_data); }
		char sbuf[32] = {0};
		snprintk(sbuf, sizeof(sbuf), "{\"adc\":%d}", valid_adc_data);
		send_queued_data_to_golioth(sbuf, "sensor");

		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return;
		}
		k_msleep(SLEEP_TIME_MS);
	}
}
