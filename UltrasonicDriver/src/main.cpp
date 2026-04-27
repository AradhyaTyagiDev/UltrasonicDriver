

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "UltrasonicDriverFactory.h"
#include "UltrasonicTypes.h"
#include "IUltrasonicDriver.h"

void systemInit();

#if defined(ESP_PLATFORM) && !defined(ARDUINO)

extern "C" void app_main(void)
{
    systemInit();
}

#elif defined(ARDUINO)

void setup()
{
    systemInit();
}

void loop()
{
}

#endif

void systemInit()
{
#if defined(ESP_PLATFORM)
    printf("Start\n");

    std::vector<UltrasonicConfig> configs = {
        {5, 18, 50, 5.0, {0.0f, 0.0f}, 'F'},
    };

    QueueHandle_t queue = xQueueCreate(32, sizeof(UltrasonicEchoEvent));

    auto ctx = createUltrasonicDriverContext(configs, queue);
    ctx.driver->begin();

    while (true)
    {
        ctx.driver->startReceive(UltrasonicSensorId::FRONT);

        UltrasonicEchoEvent evt;

        if (xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)))
        {
            // printf("Sensor %d Distance: %" PRIu32 " us\n",
            //        static_cast<int>(evt.sensorId),
            //        evt.duration);

            printf("Sensor %d Distance: %lu us\n",
                   static_cast<int>(evt.sensorId),
                   static_cast<unsigned long>(evt.duration));
        }
        else
        {
            printf("Timeout\n");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
#else
    Serial.begin(115200);
    Serial.println("Start");
#endif
}
