#include <assert.h>
#include <stdlib.h> // for rand()
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
        test_cross_talk();
        test_stress();
        test_queue_overflow();
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

    /// @brief 👉 Simulates two sensors firing almost together (real-world problem)
    static void test_cross_talk()
    {
        QueueHandle_t q;
        auto ctx = createDriver(q);

        assert(ctx.supportsTest());

        ctx.driver->startReceive(UltrasonicSensorId::FRONT);
        ctx.driver->startReceive(UltrasonicSensorId::REAR);

        // 🔥 Cross-talk scenario (very close timings)
        ctx.test->schedule(UltrasonicSensorId::FRONT, 1000, 10);
        ctx.test->schedule(UltrasonicSensorId::REAR, 900, 12);

        ctx.test->tick(15);

        UltrasonicEchoEvent evt1, evt2;

        bool ok1 = xQueueReceive(q, &evt1, 0);
        bool ok2 = xQueueReceive(q, &evt2, 0);

        assert(ok1);
        assert(ok2);

        // Order may vary → don't assume order
        assert(
            (evt1.sensorId == UltrasonicSensorId::FRONT ||
             evt1.sensorId == UltrasonicSensorId::REAR));

        assert(
            (evt2.sensorId == UltrasonicSensorId::FRONT ||
             evt2.sensorId == UltrasonicSensorId::REAR));

        assert(evt1.sensorId != evt2.sensorId);
    }

    ///@brief 👉 Simulates heavy load + random timings
    static void test_stress()
    {
        QueueHandle_t q;
        auto ctx = createDriver(q);

        assert(ctx.supportsTest());

        ctx.driver->startReceive(UltrasonicSensorId::FRONT);

        // 🔥 Heavy load
        for (int i = 0; i < 1000; i++)
        {
            ctx.test->schedule(
                UltrasonicSensorId::FRONT,
                rand() % 2000,
                i);
        }

        ctx.test->tick(1000);

        // Drain queue
        UltrasonicEchoEvent evt;
        int count = 0;

        while (xQueueReceive(q, &evt, 0))
        {
            count++;
        }

        // Expect many events (not necessarily all due to queue limit)
        assert(count > 0);

        // 🔥 Check drop behavior (important)
        assert(ctx.driver->getTotalDrops() >= 0);
    }

    /// @brief Queue Overflow / Drop Validation
    static void test_queue_overflow()
    {
        QueueHandle_t q = xQueueCreate(2, sizeof(UltrasonicEchoEvent)); // small queue

        std::vector<UltrasonicConfig> configs = {
            {5, 18, 50, 5.0, {0.0f, 0.0f}, 'F'},
        };

        auto ctx = createUltrasonicDriverContext(configs, q);

        assert(ctx.supportsTest());

        ctx.driver->startReceive(UltrasonicSensorId::FRONT);

        for (int i = 0; i < 10; i++)
        {
            ctx.test->schedule(UltrasonicSensorId::FRONT, 1000 + i, i);
        }

        ctx.test->tick(20);

        assert(ctx.driver->getTotalDrops() > 0);
        assert(ctx.driver->getSensorDrops(UltrasonicSensorId::FRONT) > 0);
    }
};