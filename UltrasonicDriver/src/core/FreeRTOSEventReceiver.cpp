#include "driver/UltrasonicDriverTypes.h"
#include "driver/FreeRTOSEventReceiver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <assert.h>

FreeRTOSEventReceiver::FreeRTOSEventReceiver(QueueHandle_t q)
    : queue(q)
{
    assert(queue != nullptr);
}

bool FreeRTOSEventReceiver::push(const UltrasonicEchoEvent &event)
{
    // Non-blocking push (important for system responsiveness)
    return xQueueSend(queue, &event, 0) == pdPASS;
}

bool FreeRTOSEventReceiver::pushFromISR(const UltrasonicEchoEvent &event)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    const bool ok =
        xQueueSendFromISR(queue, &event, &higherPriorityTaskWoken) == pdPASS;

    // 🔥 Handle context switch internally (DO NOT expose outside)
    if (higherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }

    return ok;
}
