#include "UltrasonicRMTDriver.h"

#include <assert.h>
#include "esp_err.h"
#include "UltrasonicUtils.h"

// =============================
// Constructor: Driver is only responsible for managing RMT channels and pushing events to queue. echo capture only (RMT RX).
// 🚨 Manager handles trigger
/* Manager:
        → trigger sensor (GPIO)
        → call driver.startReceive()

    Driver:
        → capture echo only
*/
// =============================
/// Create xQueueCreate(32, sizeof(UltrasonicEchoEvent)); and Pass to factory → driver
UltrasonicRMTDriver::UltrasonicRMTDriver(
    const std::vector<UltrasonicConfig> &cfg,
    QueueHandle_t queue)
    : configs(cfg), echoQueue(queue)
{
    assert(queue != nullptr);

    if (configs.empty() || configs.size() > 8)
    {
        printf("Invalid sensor count: %d\n", (int)configs.size());
        abort();
    }

    channels.resize(configs.size());
    rxBuffers.resize(configs.size());
    dropCounter.resize(configs.size(), 0);
}

// =============================
// Begin
// =============================
void UltrasonicRMTDriver::begin()
{
    for (size_t i = 0; i < configs.size(); i++)
    {
        setupRMTChannel(i);
    }
}

// =============================
// Start Receive (NON-BLOCKING): Call from manager/task after processing event.
// =============================
void UltrasonicRMTDriver::startReceive(UltrasonicSensorId sensor)
{
    const size_t idx = toIndex(sensor);

    // 🔒 Fast bounds check
    if (idx >= configs.size())
        return;

    auto ch = channels[idx];
    if (ch == nullptr)
        return;

    auto &buffer = rxBuffers[idx];
    if (buffer.empty())
        return;

    // 🔥 Static config (avoid re-init cost)
    static const rmt_receive_config_t cfg = {
        .signal_range_min_ns = 1000,
        .signal_range_max_ns = 30000000,
        .flags = {
            .en_partial_rx = 0,
        },
    };

    esp_err_t err = rmt_receive(
        ch,
        buffer.data(),
        buffer.size() * sizeof(rmt_symbol_word_t),
        &cfg);

    if (err != ESP_OK)
    {
        // ✅ Atomic increment (safe)
        __atomic_add_fetch(&errorCounter, 1, __ATOMIC_RELAXED);

        // ❌ DO NOT log here (hot path)
        // logging should be done in manager or debug mode
    }
}

// ================================
// 🔷 DROP STATS
// ================================
uint32_t UltrasonicRMTDriver::getTotalDrops() const
{
    return __atomic_load_n(&totalDrops, __ATOMIC_RELAXED);
}

uint32_t UltrasonicRMTDriver::getSensorDrops(UltrasonicSensorId sensor) const
{
    const size_t idx = toIndex(sensor);

    // 🔒 Fast bounds check
    if (idx >= dropCounter.size())
        return 0;

    return __atomic_load_n(&dropCounter[idx], __ATOMIC_RELAXED);
}

uint32_t UltrasonicRMTDriver::getErrorCount() const
{
    return __atomic_load_n(&errorCounter, __ATOMIC_RELAXED);
}

// ================= SETUP CHANNEL=================
void UltrasonicRMTDriver::setupRMTChannel(size_t i)
{
    rmt_rx_channel_config_t rx_cfg = {};
    rx_cfg.gpio_num = (gpio_num_t)configs[i].echoPin;
    rx_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
    rx_cfg.resolution_hz = 1000000; // 1 MHz → 1 tick = 1µs (HC-SR04 gives duration in µs, so 1µs resolution is ideal)
    rx_cfg.mem_block_symbols = 64;

    rmt_channel_handle_t channel = nullptr;

    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_cfg, &channel));

    channels[i] = channel;
    rxBuffers[i].resize(64);

    // ✅ Register callback per channel (IMPORTANT)
    ESP_ERROR_CHECK(
        rmt_rx_register_event_callbacks(
            channel,
            &rx_callbacks,
            this));

    ESP_ERROR_CHECK(rmt_enable(channel));
}

// ===============🔷 RMT-ISR CALLBACK=================
// 👉 IRAM_ATTR tells the compiler: “Put this function in internal RAM (IRAM) instead of Flash.”
// ✔ Function is stored in internal RAM
// ✔ Always accessible
// ✔ Safe during interrupts
/// IRAM_ATTR = Make ISR safe and crash-proof
/// ⚡ IRAM is for ISR entry, not for entire call stack
bool IRAM_ATTR UltrasonicRMTDriver::onReceiveDone(
    rmt_channel_handle_t channel,
    const rmt_rx_done_event_data_t *edata,
    void *user_ctx)
{
    auto *driver = static_cast<UltrasonicRMTDriver *>(user_ctx);

    UltrasonicEchoEvent evt;
    // Map channel → sensorId
    uint8_t ch = driver->channelToIndex(channel);
    evt.sensorId = static_cast<UltrasonicSensorId>(ch);
    // Duration in microseconds (depends on resolution). microsecond precision
    evt.duration = driver->parseDuration(edata);
    // 🔥 ISR-safe timestamp. Precision: 1–10 ms
    // if need high precision: evt.highResTime = (uint32_t)esp_timer_get_time();
    evt.timestamp = xTaskGetTickCountFromISR();
    evt.timeout = false;

    BaseType_t hpTaskWoken = pdFALSE;

    // 🔥 Push to global queue
    // ✔ ISR-safe API
    if (xQueueSendFromISR(driver->echoQueue, &evt, &hpTaskWoken) != pdPASS)
    {
        __atomic_add_fetch(&driver->dropCounter[ch], 1, __ATOMIC_RELAXED);
        __atomic_add_fetch(&driver->totalDrops, 1, __ATOMIC_RELAXED);
    }

    return hpTaskWoken == pdTRUE;
}

// ===============Static Callback Definition==============
const rmt_rx_event_callbacks_t UltrasonicRMTDriver::rx_callbacks = {
    .on_recv_done = UltrasonicRMTDriver::onReceiveDone,
};

// ================= HELPERS =================
// 👉 Finds which sensor triggered the interrupt. Linear search → fine for ≤ 8 sensors
// ISR runs when flash cache may be disabled. These functions live in flash → crash risk.
uint8_t UltrasonicRMTDriver::channelToIndex(rmt_channel_handle_t ch)
{
    for (uint8_t i = 0; i < channels.size(); i++)
    {
        if (channels[i] == ch)
            return i;
    }
    return 0; // fallback (should not happen)
}

// HC-SR04: HIGH pulse = echo duration
// RMT gives symbols (level0, duration0, level1, duration1)
// symbol is a compact way to store a piece of waveform.
// 👉 One symbol = two signal segments
// Symbol = [level0 + duration0] + [level1 + duration1]
uint32_t UltrasonicRMTDriver::parseDuration(
    const rmt_rx_done_event_data_t *edata)
{
    if (!edata || edata->num_symbols == 0)
        return 0;

    const rmt_symbol_word_t *s = edata->received_symbols;

    uint32_t duration = 0;

    // Symbol 0
    if (s[0].level0 == 1)
        duration += s[0].duration0;
    if (s[0].level1 == 1)
        duration += s[0].duration1;

    // Symbol 1 (if exists)
    if (edata->num_symbols > 1)
    {
        if (s[1].level0 == 1)
            duration += s[1].duration0;
        if (s[1].level1 == 1)
            duration += s[1].duration1;
    }

    return duration; // in µs (since resolution = 1MHz)
}
