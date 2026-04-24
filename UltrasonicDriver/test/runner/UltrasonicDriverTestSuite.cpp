#include <assert.h>
#include <UltrasonicDriverFactory.h>
#include <UltrasonicTypes.h>
#include <UltrasonicDriverContext.h>

class UltrasonicDriverTestSuite
{
public:
    static void runAll()
    {
        test_basic_echo();
        test_timeout();
        test_multi_sensor();
        test_scheduling();
    }

private:
    static UltrasonicDriverContext createDriver(QueueHandle_t &queue)
    {
        queue = xQueueCreate(32, sizeof(UltrasonicEchoEvent));

        std::vector<UltrasonicConfig> configs = {
            {5, 18, 50, 5.0, {0.0f, 0.0f}, 'F'},
        };

        // ✅ Factory already calls begin()
        return createUltrasonicDriverContext(configs, queue);
    }

    static void test_basic_echo()
    {
        QueueHandle_t q;
        auto ctx = createDriver(q);

        assert(ctx.supportsTest());

        ctx.driver->startReceive(UltrasonicSensorId::FRONT);

        ctx.test->schedule(UltrasonicSensorId::FRONT, 1200, 10);

        ctx.test->tick(10);

        UltrasonicEchoEvent evt;
        bool ok = xQueueReceive(q, &evt, 0);

        assert(ok);
        assert(evt.sensorId == UltrasonicSensorId::FRONT);
        assert(evt.duration == 1200);
        assert(!evt.timeout);
    }

    static void test_timeout()
    {
        QueueHandle_t q;
        auto ctx = createDriver(q);

        assert(ctx.supportsTest());

        ctx.driver->startReceive(UltrasonicSensorId::FRONT);

        ctx.test->tick(60);

        UltrasonicEchoEvent evt;
        xQueueReceive(q, &evt, 0);

        assert(evt.timeout == true);
    }

    static void test_multi_sensor()
    {
        QueueHandle_t q;
        auto ctx = createDriver(q);

        assert(ctx.supportsTest());

        ctx.driver->startReceive(UltrasonicSensorId::FRONT);
        ctx.driver->startReceive(UltrasonicSensorId::REAR);

        ctx.test->schedule(UltrasonicSensorId::FRONT, 1000, 10);
        ctx.test->schedule(UltrasonicSensorId::REAR, 2000, 20);

        ctx.test->tick(10);

        UltrasonicEchoEvent evt;
        xQueueReceive(q, &evt, 0);
        assert(evt.sensorId == UltrasonicSensorId::FRONT);

        ctx.test->tick(10);
        xQueueReceive(q, &evt, 0);
        assert(evt.sensorId == UltrasonicSensorId::REAR);
    }

    static void test_scheduling()
    {
        QueueHandle_t q;
        auto ctx = createDriver(q);

        assert(ctx.supportsTest());

        ctx.driver->startReceive(UltrasonicSensorId::FRONT);

        ctx.test->schedule(UltrasonicSensorId::FRONT, 1500, 50);

        ctx.test->tick(40);

        UltrasonicEchoEvent evt;
        bool ok = xQueueReceive(q, &evt, 0);

        assert(!ok); // not ready yet

        ctx.test->tick(10);

        ok = xQueueReceive(q, &evt, 0);
        assert(ok);
        assert(evt.duration == 1500);
    }
};

// // NEED TO DONE:
// ✅ Test 4 — Cross - talk simulation : 👉 Helps test manager filtering
//                         driver.schedule(0, 1000, 10);
// driver.schedule(1, 900, 12);

// ✅ Test 5 — Stress test : for (int i = 0; i < 1000; i++)
// {
//     driver.schedule(0, rand() % 2000, i);
// }

// driver.tick(1000);

// 🚀 STEP 2 — Run Tests from main(ESP - IDF or native) extern "C" void app_main()
// {
// #ifdef UNIT_TEST
//     UltrasonicMockDriverTests::runAll();
//     printf("All tests passed\n");
// #endif
// }

// 🚀 STEP 3 — Native Testing(HIGHLY RECOMMENDED)
//     [env:native] platform = native build_flags = -DUNIT_TEST

// 👉 Fast,
//                  no hardware needed
//                      Run : pio run
//                            -
//                            e native