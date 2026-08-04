#ifndef MIC_DRIVER_H
#define MIC_DRIVER_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initialize the onboard digital PDM microphone and turn on its power regulator.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int mic_drv_init(void);

/**
 * @brief Start capturing audio from the PDM microphone.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int mic_drv_start(void);

/**
 * @brief Stop capturing audio.
 * 
 * @return int 0 on success, negative errno on failure.
 */
int mic_drv_stop(void);

/**
 * @brief Read a block of audio samples (blocking).
 * 
 * @param buffer Buffer to store 16-bit PCM samples.
 * @param samples Size of buffer in samples.
 * @return int 0 on success, negative errno on failure.
 */
int mic_drv_read(int16_t *buffer, size_t samples);

#endif // MIC_DRIVER_H
