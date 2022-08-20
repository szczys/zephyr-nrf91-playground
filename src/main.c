/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(basic_spi, LOG_LEVEL_DBG);

#define SPI_OP	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/* https://stackoverflow.com/a/69152884 */
#include <drivers/gpio.h>
#include <drivers/spi.h>

/* likely we could use this example:
 * https://docs.zephyrproject.org/latest/hardware/peripherals/spi.html#c.SPI_CS_CONTROL_PTR_DT
 * Instead of the following code: */

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
	LOG_INF("Started SPI demo");
	int ret;

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
	
	uint8_t my_buffer[10] = {0};
	struct spi_buf my_spi_buffer[1];
	my_spi_buffer[0].buf = my_buffer;
	my_spi_buffer[0].len = 10;
	const struct spi_buf_set rx_buff = { my_spi_buffer, 1 };

	while (1) {
		ret = spi_read_dt(&mcp3201_dev, &rx_buff);
		LOG_INF("spi_read status: %d", ret);
		LOG_INF("Received 10 bytes:");
		LOG_INF("%d %d %d %d %d %d %d %d %d %d",
					my_buffer[0],my_buffer[1],my_buffer[2],
					my_buffer[3],my_buffer[4],my_buffer[5],
					my_buffer[6],my_buffer[7],my_buffer[8],
					my_buffer[9]
				);

		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return;
		}
		k_msleep(SLEEP_TIME_MS);
	}
}
