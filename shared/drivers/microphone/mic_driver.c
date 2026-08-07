#include "mic_driver.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(mic_driver, LOG_LEVEL_INF);

#define MIC_PWR_PIN 10
#define BLOCK_SIZE  512 // 512 bytes = 256 16-bit PCM samples (matches ADPCM block size)
#define BLOCK_COUNT 16  // 16 slab buffers to prevent PDM DMA buffer exhaustion

static const struct device *gpio1_dev = NULL;
static const struct device *pdm_dev = NULL;

K_MEM_SLAB_DEFINE(mic_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

static struct pcm_stream_cfg stream_cfg = {
    .pcm_rate = 16000,
    .pcm_width = 16,
    .block_size = BLOCK_SIZE,
    .mem_slab = &mic_slab,
};

static struct dmic_cfg mic_config = {
    .io = {
        .min_pdm_clk_freq = 1000000,
        .max_pdm_clk_freq = 3250000,
        .min_pdm_clk_dc = 40,
        .max_pdm_clk_dc = 60,
    },
    .streams = &stream_cfg,
    .channel = {
        .req_chan_map_lo = 0,
        .req_num_chan = 1,
        .req_num_streams = 1,
    },
};

int mic_drv_init(void)
{
    int ret;

    gpio1_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio1));
    if (gpio1_dev == NULL || !device_is_ready(gpio1_dev)) {
        LOG_ERR("GPIO1 device not ready for microphone");
        return -ENODEV;
    }

    pdm_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(pdm0));
    if (pdm_dev == NULL || !device_is_ready(pdm_dev)) {
        LOG_ERR("PDM0 device not ready");
        return -ENODEV;
    }

    // Configure microphone power enable pin (output, default high / on)
    ret = gpio_pin_configure(gpio1_dev, MIC_PWR_PIN, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure mic power pin (err %d)", ret);
        return ret;
    }

    LOG_INF("Microphone driver successfully initialized");
    return 0;
}

int mic_drv_start(void)
{
    int ret;

    if (gpio1_dev == NULL || pdm_dev == NULL) {
        return -EINVAL;
    }

    // Power ON the PDM microphone (GPIO Port 1 Pin 10 HIGH)
    ret = gpio_pin_set(gpio1_dev, MIC_PWR_PIN, 1);
    if (ret < 0) {
        LOG_ERR("Failed to enable microphone power");
        return ret;
    }

    // Wait for the microphone power rail to stabilize
    k_msleep(20);

    // On XIAO BLE Sense, PDM mic data line is active on RIGHT channel (or LEFT fallback)
    mic_config.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT);

    ret = dmic_configure(pdm_dev, &mic_config);
    if (ret < 0) {
        LOG_WRN("Configuring PDM_CHAN_RIGHT failed (%d), trying PDM_CHAN_LEFT...", ret);
        mic_config.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
        ret = dmic_configure(pdm_dev, &mic_config);
        if (ret < 0) {
            LOG_ERR("Failed to configure PDM DMIC (err %d)", ret);
            return ret;
        }
    }

    ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_START);
    if (ret < 0) {
        LOG_ERR("Failed to trigger PDM DMIC start (err %d)", ret);
        return ret;
    }

    LOG_INF("Microphone PDM recording started successfully");
    return 0;
}

int mic_drv_stop(void)
{
    int ret = 0;

    if (pdm_dev != NULL) {
        ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_STOP);
        if (ret < 0) {
            LOG_ERR("Failed to trigger PDM DMIC stop (err %d)", ret);
        }
    }

    LOG_INF("Microphone recording stopped");
    return ret;
}

int mic_drv_read(int16_t *buffer, size_t samples)
{
    if (pdm_dev == NULL || buffer == NULL || samples == 0) {
        return -EINVAL;
    }

    void *mem_slice;
    size_t size;
    int ret = dmic_read(pdm_dev, 0, &mem_slice, &size, 100);
    if (ret < 0) {
        return ret;
    }

    size_t bytes_to_copy = size < (samples * sizeof(int16_t)) ? size : (samples * sizeof(int16_t));
    memcpy(buffer, mem_slice, bytes_to_copy);

    k_mem_slab_free(&mic_slab, mem_slice);

    return 0;
}
