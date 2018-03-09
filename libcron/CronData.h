#pragma once

#include <set>
#include <regex>
#include <string>
#include <vector>
#include "TimeTypes.h"

namespace libcron
{
    /*
        This class parses strings in the format specified below and holds the resulting allowed values
        in its internal sets.

        Cron format, 6 parts:

       ┌──────────────seconds (0 - 59)
       │ ┌───────────── minute (0 - 59)
       │ │ ┌───────────── hour (0 - 23)
       │ │ │ ┌───────────── day of month (1 - 31)
       │ │ │ │ ┌───────────── month (1 - 12)
       │ │ │ │ │ ┌───────────── day of week (0 - 6) (Sunday to Saturday;
       │ │ │ │ │ │                                       7 is also Sunday on some systems)
       │ │ │ │ │ │
       │ │ │ │ │ │
       * * * * * *

        Allowed formats:
            Special characters: '*', meaning the entire range.

            Ranges: 1,2,4-6
                Result: 1,2,4,5,6
            Steps:  1/2
                Result: 1,3,5,7...<max>

            For day of month, these strings are valid, case insensitive:
            SUN, MON, TUE, WED, THU, FRI, SAT
                Example: MON-THU,SAT

            For month, these strings are valid, case insensitive:
            JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC
                Example: JAN,MAR,SEP-NOV

            Each part is separated by one or more whitespaces. It is thus important to keep
            whitespaces out of the respective parts.

            Valid:
                * * * * * *
                0,3,40-50 * * * * *

            Invalid:
                0, 3, 40-50 * * * * *

     */

    class CronData
    {
        public:
            static CronData create(const std::string& cron_expression);

            CronData();

            bool is_valid() const
            {
                return valid;
            }

            const std::set<Seconds>& get_seconds() const
            {
                return seconds;
            }

            const std::set<Minutes>& get_minutes() const
            {
                return minutes;
            }

            const std::set<Hours>& get_hours() const
            {
                return hours;
            }

            const std::set<DayOfMonth>& get_day_of_month() const
            {
                return day_of_month;
            }

            const std::set<Months>& get_months() const
            {
                return months;
            }

            const std::set<DayOfWeek>& get_day_of_week() const
            {
                return day_of_week;
            }

            template<typename T>
            static uint8_t value_of(T t)
            {
                return static_cast<uint8_t>(t);
            }

        private:

            void parse(const std::string& cron_expression);

            template<typename T>
            bool validate_numeric(const std::string& s, std::set<T>& numbers);

            template<typename T>
            bool validate_literal(const std::string& s,
                                  std::set<T>& numbers,
                                  const std::vector<std::string>& names);

            template<typename T>
            bool process_parts(const std::vector<std::string>& parts, std::set<T>& numbers);

            template<typename T>
            bool add_number(std::set<T>& set, int32_t number);

            template<typename T>
            bool is_within_limits(int32_t low, int32_t high);

            template<typename T>
            bool get_range(const std::string& s, T& low, T& high);

            template<typename T>
            bool get_step(const std::string& s, uint8_t& start, uint8_t& step);

            std::vector<std::string> split(const std::string& s, char token);

            bool is_number(const std::string& s);

            bool is_between(int32_t value, int32_t low_limit, int32_t high_limit);

            std::set<Seconds> seconds{};
            std::set<Minutes> minutes{};
            std::set<Hours> hours{};
            std::set<DayOfMonth> day_of_month{};
            std::set<Months> months{};
            std::set<DayOfWeek> day_of_week{};
            bool valid = false;

            std::vector<std::string> month_names;
            std::vector<std::string> day_names;

            template<typename T>
            void add_full_range(std::set<T>& set);
    };

    template<typename T>
    bool CronData::validate_numeric(const std::string& s, std::set<T>& numbers)
    {
        std::vector<std::string> parts = split(s, ',');

        return process_parts(parts, numbers);
    }

    template<typename T>
    bool CronData::validate_literal(const std::string& s,
                                    std::set<T>& numbers,
                                    const std::vector<std::string>& names)
    {
        std::vector<std::string> parts = split(s, ',');

        auto value_of_first_name = value_of(T::First);

        // Replace each found name with the corresponding value.
        for (const auto& name : names)
        {
            std::regex m(name, std::regex_constants::ECMAScript | std::regex_constants::icase);
            for (size_t i = 0; i < parts.size(); ++i)
            {
                std::string replaced;
                std::regex_replace(std::back_inserter(replaced), parts[i].begin(), parts[i].end(), m,
                                   std::to_string(value_of_first_name));

                parts[i] = replaced;
            }

            value_of_first_name++;
        }

        return process_parts(parts, numbers);

    }

    template<typename T>
    bool CronData::process_parts(const std::vector<std::string>& parts, std::set<T>& numbers)
    {
        bool res = true;

        T left;
        T right;
        uint8_t step_start;
        uint8_t step;

        for (const auto& p : parts)
        {
            if (p == "*")
            {
                add_full_range<T>(numbers);
            }
            else if (is_number(p))
            {
                res &= add_number<T>(numbers, std::stoi(p));
            }
            else if (get_range<T>(p, left, right))
            {
                // A range can be written as both 1-22 or 22-1, meaning totally different ranges.
                // First case is 1...22 while 22-1 is only four hours: 22, 23, 0, 1.
                if (left <= right)
                {
                    for (auto v = value_of(left); v <= value_of(right); ++v)
                    {
                        res &= add_number(numbers, v);
                    }
                }
                else
                {
                    // 'left' and 'right' are not in value order. First, get values between 'left' and T::Last, inclusive
                    for (auto v = value_of(left); v <= value_of(T::Last); ++v)
                    {
                        res &= add_number(numbers, v);
                    }

                    // Next, get values between T::First and 'right', inclusive.
                    for (auto v = value_of(T::First); v <= value_of(right); ++v)
                    {
                        res &= add_number(numbers, v);
                    }
                }
            }
            else if (get_step<T>(p, step_start, step))
            {
                // Add from step_start to T::Last with a step of 'step'
                for (auto v = step_start; v <= value_of(T::Last); v += step)
                {
                    res &= add_number(numbers, v);
                }
            }
            else
            {
                res = false;
            }
        }

        return res;
    }

    template<typename T>
    bool CronData::get_range(const std::string& s, T& low, T& high)
    {
        bool res = false;

        auto value_range = R"#((\d+)-(\d+))#";

        std::regex range(value_range, std::regex_constants::ECMAScript);

        std::smatch match;

        if (std::regex_match(s.begin(), s.end(), match, range))
        {
            auto left = std::stoi(match[1].str().c_str());
            auto right = std::stoi(match[2].str().c_str());

            if (is_within_limits<T>(left, right))
            {
                low = static_cast<T>(left);
                high = static_cast<T>(right);
                res = true;
            }
        }

        return res;
    }

    template<typename T>
    bool CronData::get_step(const std::string& s, uint8_t& start, uint8_t& step)
    {
        bool res = false;

        auto value_range = R"#((\d+)/(\d+))#";

        std::regex range(value_range, std::regex_constants::ECMAScript);

        std::smatch match;

        if (std::regex_match(s.begin(), s.end(), match, range))
        {
            auto raw_start = std::stoi(match[1].str().c_str());
            auto raw_step = std::stoi(match[2].str().c_str());

            if (is_within_limits<T>(raw_start, raw_start) && raw_step > 0)
            {
                start = static_cast<uint8_t>(raw_start);
                step = static_cast<uint8_t>(raw_step);
                res = true;
            }
        }

        return res;
    }

    template<typename T>
    void CronData::add_full_range(std::set<T>& set)
    {
        for (auto v = value_of(T::First); v <= value_of(T::Last); ++v)
        {
            if (set.find(static_cast<T>(v)) == set.end())
            {
                set.emplace(static_cast<T>(v));
            }
        }
    }

    template<typename T>
    bool CronData::add_number(std::set<T>& set, int32_t number)
    {
        bool res = true;

        // Don't add if already there
        if (set.find(static_cast<T>(number)) == set.end())
        {
            // Check range
            if (is_within_limits<T>(number, number))
            {
                set.emplace(static_cast<T>(number));
            }
            else
            {
                res = false;
            }
        }

        return res;
    }

    template<typename T>
    bool CronData::is_within_limits(int32_t low, int32_t high)
    {
        return is_between(low, value_of(T::First), value_of(T::Last))
               && is_between(high, value_of(T::First), value_of(T::Last));
    }


}
