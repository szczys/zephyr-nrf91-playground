/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/* https://stackoverflow.com/a/69152884 */
#include <drivers/gpio.h>
#include <drivers/spi.h>

const struct device *spi_dev;

/* likely we could use this example:
 * https://docs.zephyrproject.org/latest/hardware/peripherals/spi.html#c.SPI_CS_CONTROL_PTR_DT
 * Instead of the following code: */

struct spi_cs_control spi_cs = {
    /* PA4 as CS pin */
    .gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .gpio_pin = 18,
    .gpio_dt_flags = GPIO_ACTIVE_LOW,
    /* delay in microseconds to wait before starting the transmission and before releasing the CS line */
    .delay = 10,
};

#define SPI_CS (&spi_cs)

struct spi_config spi_cfg = {
    .frequency = 350000,
    .operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_LOCK_ON,
    .cs = SPI_CS,
};

void spi_init()
{
    spi_dev = device_get_binding("SPI_2");
}

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
	printk("Started SPI demo\n");
	int ret;

	if (!device_is_ready(led.port)) {
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}

	uint8_t my_buffer[10] = {0};
	struct spi_buf my_spi_buffer = { my_buffer, 10 };
	struct spi_buf_set rx_buff = { &my_spi_buffer, 10 };
	spi_init();

// 	const struct spi_dt_spec *spi_dev = SPI_DT_SPEC_GET(DT_NODELABEL(spidev),
// 			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) |
// 			SPI_LINES_SINGLE | SPI_LOCK_ON,
// 			10);

	
	while (1) {
// 		gpio_pin_set_dt(&led, 0);
// 		k_sleep(K_MSEC(100));
		spi_read(spi_dev, &spi_cfg, &rx_buff);
		printk("Received 10 bytes:");
		printk("%d %d %d %d %d %d %d %d %d %d\n",
					my_buffer[0],my_buffer[1],my_buffer[2],
					my_buffer[3],my_buffer[4],my_buffer[5],
					my_buffer[6],my_buffer[7],my_buffer[8],
					my_buffer[9]
				);
// 		gpio_pin_set_dt(&led, 1);

	// 101 111010111 101
// 		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return;
		}
		k_msleep(SLEEP_TIME_MS);
	}
}
