#pragma once

#include "libcron/CronData.h"
#include <chrono>
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4244)
#endif
#ifdef __cplusplus > 201703L
#else
#include <date/date.h>
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "libcron/DateTime.h"

namespace libcron
{
    class CronSchedule
    {
        public:
            explicit CronSchedule(CronData& data)
                    : data(data)
            {
            }

            CronSchedule(const CronSchedule&) = default;

            CronSchedule& operator=(const CronSchedule&) = default;

            std::tuple<bool, std::chrono::system_clock::time_point>
            calculate_from(const std::chrono::system_clock::time_point& from) const;

            // https://github.com/HowardHinnant/date/wiki/Examples-and-Recipes#obtaining-ymd-hms-components-from-a-time_point
            static DateTime to_calendar_time(std::chrono::system_clock::time_point time)
            {
#ifdef __cplusplus > 201703L
                auto daypoint = std::chrono::floor<std::chrono::days>(time);
                auto ymd = std::chrono::year_month_day(daypoint);   // calendar date
                auto time_of_day = std::chrono::hh_mm_ss(time - daypoint);
#else
                auto daypoint = date::floor<date::days>(time);
                auto ymd = date::year_month_day(daypoint);   // calendar date
                auto time_of_day = date::make_time(time - daypoint); // Yields time_of_day type
#endif
                // Obtain individual components as integers
                DateTime dt{
                        static_cast<int>(ymd.year()),
                        static_cast<unsigned>(ymd.month()),
                        static_cast<unsigned>(ymd.day()),
                        static_cast<uint8_t>(time_of_day.hours().count()),
                        static_cast<uint8_t>(time_of_day.minutes().count()),
                        static_cast<uint8_t>(time_of_day.seconds().count())};

                return dt;
            }

        private:
            CronData data;
    };

}
