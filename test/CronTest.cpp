#include <catch.hpp>
#include <libcron/Cron.h>
#include <thread>
#include <iostream>

using namespace libcron;
using namespace std::chrono;

std::string create_schedule_expiring_in(hours h, minutes m, seconds s)
{
    auto now = system_clock::now() + h + m + s;
    auto dt = CronSchedule::to_calendar_time(now);

    std::string res{};
    res += std::to_string(dt.sec) + " ";
    res += std::to_string(dt.min) + " ";
    res += std::to_string(dt.hour) + " * * ?";

    return res;
}


SCENARIO("Adding a task")
{
    GIVEN("A Cron instance with no task")
    {
        Cron c;
        auto expired = false;

        THEN("Starts with no task")
        {
            REQUIRE(c.count() == 0);
        }

        WHEN("Adding a task that runs every second")
        {
            REQUIRE(c.add_schedule("A task", "* * * * * ?",
                                   [&expired]()
                                   {
                                       expired = true;
                                   })
            );

            THEN("Count is 1 and task was not expired two seconds ago")
            {
                REQUIRE(c.count() == 1);
                c.execute_expired_tasks(system_clock::now() - 2s);
                REQUIRE_FALSE(expired);
            }
            AND_THEN("Task is expired when calculating based on current time")
            {
                c.execute_expired_tasks();
                THEN("Task is expired")
                {
                    REQUIRE(expired);
                }
            }
        }
    }
}

SCENARIO("Adding a task that expires in the future")
{
    GIVEN("A Cron instance with task expiring in 3 seconds")
    {
        auto expired = false;

        Cron c;
        REQUIRE(c.add_schedule("A task", create_schedule_expiring_in(hours{0}, minutes{0}, seconds{3}),
                               [&expired]()
                               {
                                   expired = true;
                               })
        );

        THEN("Not yet expired")
        {
            REQUIRE_FALSE(expired);
        }
        AND_WHEN("When waiting one second")
        {
            std::this_thread::sleep_for(1s);
            c.execute_expired_tasks();
            THEN("Task has not yet expired")
            {
                REQUIRE_FALSE(expired);
            }
        }
        AND_WHEN("When waiting three seconds")
        {
            std::this_thread::sleep_for(3s);
            c.execute_expired_tasks();
            THEN("Task has expired")
            {
                REQUIRE(expired);
            }
        }
    }
}

SCENARIO("Task priority")
{
    GIVEN("A Cron instance with two tasks expiring in 3 and 5 seconds, added in 'reverse' order")
    {
        auto _3_second_expired = 0;
        auto _5_second_expired = 0;


        Cron c;
        REQUIRE(c.add_schedule("Five", create_schedule_expiring_in(hours{0}, minutes{0}, seconds{5}),
                               [&_5_second_expired]()
                               {
                                   _5_second_expired++;
                               })
        );

        REQUIRE(c.add_schedule("Three", create_schedule_expiring_in(hours{0}, minutes{0}, seconds{3}),
                               [&_3_second_expired]()
                               {
                                   _3_second_expired++;
                               })
        );

        THEN("Not yet expired")
        {
            REQUIRE_FALSE(_3_second_expired);
            REQUIRE_FALSE(_5_second_expired);
        }

        WHEN("Waiting 1 seconds")
        {
            std::this_thread::sleep_for(1s);
            c.execute_expired_tasks();

            THEN("Task has not yet expired")
            {
                REQUIRE(_3_second_expired == 0);
                REQUIRE(_5_second_expired == 0);
            }
        }
        AND_WHEN("Waiting 3 seconds")
        {
            std::this_thread::sleep_for(3s);
            c.execute_expired_tasks();

            THEN("3 second task has expired")
            {
                REQUIRE(_3_second_expired == 1);
                REQUIRE(_5_second_expired == 0);
            }
        }
        AND_WHEN("Waiting 5 seconds")
        {
            std::this_thread::sleep_for(5s);
            c.execute_expired_tasks();

            THEN("3 and 5 second task has expired")
            {
                REQUIRE(_3_second_expired == 1);
                REQUIRE(_5_second_expired == 1);
            }
        }
        AND_WHEN("Waiting based on the time given by the Cron instance")
        {
            std::this_thread::sleep_for(c.time_until_next());
            c.execute_expired_tasks();

            THEN("3 second task has expired")
            {
                REQUIRE(_3_second_expired == 1);
                REQUIRE(_5_second_expired == 0);
            }
        }
        AND_WHEN("Waiting based on the time given by the Cron instance")
        {
            std::this_thread::sleep_for(c.time_until_next());
            REQUIRE(c.execute_expired_tasks() == 1);

            std::this_thread::sleep_for(c.time_until_next());
            REQUIRE(c.execute_expired_tasks() == 1);

            THEN("3 and 5 second task has each expired once")
            {
                REQUIRE(_3_second_expired == 1);
                REQUIRE(_5_second_expired == 1);
            }
        }

    }
}