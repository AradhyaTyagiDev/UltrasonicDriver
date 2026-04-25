#pragma once

#include <vector>
#include <stdint.h>

#include "driver/rmt_rx.h"
#include "driver/rmt_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "IUltrasonicDriver.h"
#include "UltrasonicTypes.h"

class UltrasonicRMTDriver : public IUltrasonicDriver
{
public:
    UltrasonicRMTDriver(const std::vector<UltrasonicConfig> &cfg,
                        QueueHandle_t queue);

    // IUltrasonicDriver interface
    void begin() override;
    void startReceive(UltrasonicSensorId sensor) override;

    // Stats
    uint32_t getTotalDrops() const;
    uint32_t getSensorDrops(UltrasonicSensorId sensor) const;
    uint32_t getErrorCount() const;

private:
    // ================= CONFIG =================
    std::vector<UltrasonicConfig> configs;
    QueueHandle_t echoQueue = nullptr;

    // ================= RUNTIME =================
    std::vector<rmt_channel_handle_t> channels;
    // Each sensor gets its own buffer (NO dynamic alloc)→ no collision
    std::vector<std::vector<rmt_symbol_word_t>> rxBuffers;

    // =================CACHE + DROP TRACKING + ERROR TRACKING===============
    std::vector<uint32_t> dropCounter; // dropCounter per sensor
    uint32_t totalDrops = 0;           // global
    uint32_t errorCounter = 0;

    // ================= SETUP =================
    void setupRMTChannel(size_t i);

    // ================= HELPERS =================
    uint8_t channelToIndex(rmt_channel_handle_t ch);
    uint32_t parseDuration(const rmt_rx_done_event_data_t *edata);

    // ================= ISR =================
    static bool IRAM_ATTR onReceiveDone(
        rmt_channel_handle_t channel,
        const rmt_rx_done_event_data_t *edata,
        void *user_ctx);

    // ⚠️ Declaration only (definition in .cpp)
    static const rmt_rx_event_callbacks_t rx_callbacks;
};
