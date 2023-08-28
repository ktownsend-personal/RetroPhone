
#include "DACOutput.h"

void DACOutput::start(int sample_rate)
{
    // i2s config for writing both channels of I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = (uint32_t)sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,

        // When true I2S detects not feeding buffer and sets output to 0v (-128), but that creates a sharp transition (pop/click sound).
        // We are using false and stuffing the buffer with zeros when finished feeding audio to maintain 0 signal level rather than 0v level.
        // This works because I2S continues to play the buffer loop indefinitely and will stay at 0 signal if that's what's in the buffer.
        .tx_desc_auto_clear = false,

        .fixed_mclk = 0};
    //install and start i2s driver
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    // enable the DAC channels
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    // // clear the DMA buffers
    // i2s_zero_dma_buffer(I2S_NUM_0);
    // // let it rip!
    // i2s_start(I2S_NUM_0);
}
