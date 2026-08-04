#include "rgb_led_driver.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rgb_led, LOG_LEVEL_INF);

#define RED_NODE   DT_ALIAS(led0)
#define GREEN_NODE DT_ALIAS(led1)
#define BLUE_NODE  DT_ALIAS(led2)

#if DT_NODE_EXISTS(RED_NODE) && DT_NODE_EXISTS(GREEN_NODE) && DT_NODE_EXISTS(BLUE_NODE)
static const struct gpio_dt_spec red_led   = GPIO_DT_SPEC_GET(RED_NODE, gpios);
static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(GREEN_NODE, gpios);
static const struct gpio_dt_spec blue_led  = GPIO_DT_SPEC_GET(BLUE_NODE, gpios);
#define RGB_LEDS_AVAILABLE 1
#else
#define RGB_LEDS_AVAILABLE 0
#endif

int rgb_led_init(void)
{
#if RGB_LEDS_AVAILABLE
    int ret;

    if (!gpio_is_ready_dt(&red_led) || !gpio_is_ready_dt(&green_led) || !gpio_is_ready_dt(&blue_led)) {
        LOG_ERR("RGB LED GPIO ports are not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;

    ret = gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;

    ret = gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;

    LOG_INF("RGB LED driver successfully initialized");
    return 0;
#else
    LOG_WRN("RGB LEDs are not defined in devicetree");
    return -ENODEV;
#endif
}

int rgb_led_set(bool r, bool g, bool b)
{
#if RGB_LEDS_AVAILABLE
    int ret;
    ret = gpio_pin_set_dt(&red_led, r ? 1 : 0);
    if (ret < 0) return ret;

    ret = gpio_pin_set_dt(&green_led, g ? 1 : 0);
    if (ret < 0) return ret;

    ret = gpio_pin_set_dt(&blue_led, b ? 1 : 0);
    if (ret < 0) return ret;

    return 0;
#else
    return -ENODEV;
#endif
}

int rgb_led_toggle(bool r, bool g, bool b)
{
#if RGB_LEDS_AVAILABLE
    int ret;
    if (r) {
        ret = gpio_pin_toggle_dt(&red_led);
        if (ret < 0) return ret;
    }
    if (g) {
        ret = gpio_pin_toggle_dt(&green_led);
        if (ret < 0) return ret;
    }
    if (b) {
        ret = gpio_pin_toggle_dt(&blue_led);
        if (ret < 0) return ret;
    }
    return 0;
#else
    return -ENODEV;
#endif
}

int rgb_led_set_mask(uint8_t color_mask)
{
    return rgb_led_set((color_mask & 0x01) != 0,
                       (color_mask & 0x02) != 0,
                       (color_mask & 0x04) != 0);
}
