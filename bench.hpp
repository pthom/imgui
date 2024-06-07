#pragma once
//
// bench.hpp: Dead-simple benchmark utility (header only)
//
// Pascal Thomet / 2024
//     Originally developed by
//     for FunctionalPlus (https://github.com/Dobiasd/FunctionalPlus)
//
//
// Example usage:
// --------------
//
//static BenchmarkSession gBenchmark;
//#include <vector>
//
//
//int main()
//{
//    // Create a random huge vector
//    auto CreateRandomVector = []
//    {
//        std::vector<double> randomValues;
//        for (int i = 0; i < 1000 * 1000; ++i)
//            randomValues.push_back(rand() % 1000);
//        return randomValues;
//    };
//
//    // Run the benchmark several times
//    for (int i = 0; i < 100; ++i)
//    {
//        // Create a new random vector each time
//        auto randomValues = CreateRandomVector();
//
//        // Benchmark the sort function
//        BENCHMARK_VOID_EXPRESSION(gBenchmark, "Sort",
//        {
//            std::sort(randomValues.begin(), randomValues.end());
//        });
//    }
//
//    printf("Benchmark \n%s", gBenchmark.Report().c_str());
//}



//
//  See public API at the end
//

#include <map>
#include <chrono>
#include <iomanip>
#include <vector>
#include <sstream>
#include <mutex>


namespace Bench
{
    using ExecutionTime = double;
    using FunctionName = std::string;

    
    inline double Sum(const std::vector<double>& xs)
    {
        double result = 0.0;
        for (const double x : xs)
        {
            result += x;
        }
        return result;
    }

    struct MeanAndStdDev
    {
        double Mean;
        double StdDev;
    };

    inline MeanAndStdDev ComputeMeanStdDev(const std::vector<double>& xs)
    {
        const double mean = Sum(xs) / static_cast<double>(xs.size());
        double variance = 0.0;
        for (const double x : xs)
        {
            variance += (x - mean) * (x - mean);
        }
        variance /= static_cast<double>(xs.size());
        return {mean, std::sqrt(variance)};
    }


    template<typename T, typename U>
    inline std::vector<std::pair<T, U>> MapToPairs(const std::map<T, U>& m)
    {
        std::vector<std::pair<T, U>> result;
        for (const auto& kv : m)
        {
            result.push_back(kv);
        }
        return result;
    }

    template<typename T, typename CmpFunction>
    inline std::vector<T> SortBy(const CmpFunction& is_less, const std::vector<T>& xs)
    {
        auto ys = xs;
        std::sort(ys.begin(), ys.end(), is_less);
        return ys;
    }


    class HighResTimer
    {
    public:
        HighResTimer() : _StartTime(clock::now()) {}
        void Reset() { _StartTime = clock::now(); }

        // time since creation or last reset in seconds
        double Elapsed() const
        {
            return std::chrono::duration_cast<second>(clock::now() - _StartTime).count();
        }

    private:
        typedef std::chrono::high_resolution_clock clock;
        typedef std::chrono::duration<double, std::ratio<1>> second;
        std::chrono::time_point<clock> _StartTime;
    };


    struct BenchmarkFunctionReport
    {
        std::size_t NbCalls;
        ExecutionTime TotalTime;
        ExecutionTime AverageTime;
        ExecutionTime Deviation;
    };

    namespace Internal
    {
        std::string ShowBenchmarkFunctionReport(
            const std::map<FunctionName, BenchmarkFunctionReport>& reports);
    }

    // BenchmarkSession stores timings during a benchmark session
    // and is able to emit a report at the end
    class BenchmarkSession
    {
    public:
        BenchmarkSession()
            : mFunctionTimesMutex()
            , mFunctionTimes() {};

        inline std::string Report() const
        {
            const auto reports = ReportList();
            return Bench::Internal::ShowBenchmarkFunctionReport(reports);
        }

        std::map<FunctionName, BenchmarkFunctionReport> ReportList() const
        {
            std::lock_guard<std::mutex> lock(mFunctionTimesMutex);
            std::map<FunctionName, BenchmarkFunctionReport> report;
            for (const auto& one_function_time : mFunctionTimes) {
                report[one_function_time.first] = _MakeBenchReport(one_function_time.second);
            }
            return report;
        }

        inline void StoreOneTime(const FunctionName& function_name, ExecutionTime time)
        {
            std::lock_guard<std::mutex> lock(mFunctionTimesMutex);
            mFunctionTimes[function_name].push_back(time);
        }

    private:
        BenchmarkFunctionReport _MakeBenchReport(
            const std::vector<ExecutionTime>& times) const
        {
            BenchmarkFunctionReport result;
            result.NbCalls = times.size();
            auto mean_and_dev = Bench::ComputeMeanStdDev(times);
            result.AverageTime = mean_and_dev.Mean;
            result.Deviation = mean_and_dev.StdDev;
            result.TotalTime = Bench::Sum(times);
            return result;
        }

        mutable std::mutex mFunctionTimesMutex;
        std::map<FunctionName, std::vector<ExecutionTime>> mFunctionTimes;
    };

    namespace Internal 
    {
        template <typename Fn>
        class BenchFunctionImpl
        {
        public:
            explicit BenchFunctionImpl(
                BenchmarkSession& benchmark_sess,
                FunctionName function_name,
                Fn fn)
                : benchmark_session_(benchmark_sess)
                , function_name_(function_name)
                , fn_(fn) {};

            template <typename... Args>
            auto operator()(Args&&... args)
            {
                return _BenchResult(std::forward<Args>(args)...);
            }

        private:
            template <typename... Args>
            auto _BenchResult(Args&&... args)
            {
                Bench::HighResTimer timer;
                auto r = fn_(std::forward<Args>(args)...);
                benchmark_session_.StoreOneTime(function_name_, timer.Elapsed());
                return r;
            }

            BenchmarkSession& benchmark_session_;
            FunctionName function_name_;
            Fn fn_;
        };

        template <typename Fn>
        class BenchVoidFunctionImpl
        {
        public:
            explicit BenchVoidFunctionImpl(
                BenchmarkSession& benchmark_sess,
                FunctionName function_name,
                Fn fn)
                : benchmark_session_(benchmark_sess)
                , function_name_(function_name)
                , fn_(fn) {};

            template <typename... Args>
            auto operator()(Args&&... args)
            {
                _BenchResult(std::forward<Args>(args)...);
            }

        private:
            template <typename... Args>
            auto _BenchResult(Args&&... args)
            {
                Bench::HighResTimer timer;
                fn_(std::forward<Args>(args)...);
                benchmark_session_.StoreOneTime(function_name_, timer.Elapsed());
            }

            BenchmarkSession& benchmark_session_;
            FunctionName function_name_;
            Fn fn_;
        };

    } // namespace Internal


    template <class Fn>
    auto MakeBenchmarkFunction(BenchmarkSession& session, const FunctionName& name, Fn f)
    {
        // transforms f into a function with the same
        // signature, that will store timings into the benchmark session
        return Internal::BenchFunctionImpl<Fn>(session, name, f);
    }

    template <class Fn>
    auto MakeBenchmarkVoidFunction(BenchmarkSession& session, const FunctionName& name, Fn f)
    {
        // transforms a void returning function into a function with the same
        // signature, that will store timings into the benchmark session
        return Internal::BenchVoidFunctionImpl<Fn>(session, name, f);
    }


    namespace Internal
    {
        inline std::vector<std::pair<FunctionName, BenchmarkFunctionReport>> make_ordered_reports(
            const std::map<FunctionName, BenchmarkFunctionReport>& report_map)
        {
            auto report_pairs = Bench::MapToPairs(report_map);
            auto fnCompareTotalTime = [](const auto& a, const auto& b) {
                return a.second.TotalTime > b.second.TotalTime;
            };
            auto report_pairs_sorted = Bench::SortBy(fnCompareTotalTime, report_pairs);
            return report_pairs_sorted;
        }

        inline std::string ShowBenchmarkFunctionReport(const std::map<FunctionName, BenchmarkFunctionReport>& reports)
        {
            auto ordered_reports = make_ordered_reports(reports);

            auto my_show_time_ms = [](double time) -> std::string
            {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(3);
                ss << (time * 1000.);
                return ss.str() + "ms";
            };

            auto my_show_time_us = [](double time) -> std::string
            {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(3);
                ss << (time * 1000000.);
                return ss.str() + "us";
            };

            std::stringstream ss;
            for (const auto& kv: ordered_reports)
            {
                const auto& report = kv.second;
                const auto& function_name = kv.first;
                ss << function_name << " : " << report.NbCalls << " calls, "
                   << my_show_time_ms(report.TotalTime) << " total, "
                   << my_show_time_us(report.AverageTime) << " average, "
                   << my_show_time_us(report.Deviation) << " Deviation" << std::endl;
            }
            return ss.str();
        }
    } // namespace Internal

} // namespace Bench


//
//  Public API
//
using Bench::BenchmarkSession;

#define BENCHMARK_EXPRESSION(bench_session, name, expression) \
    MakeBenchmarkFunction(                                  \
        bench_session,                                        \
        name,                                                 \
        [&]() { return expression; })();

#define BENCHMARK_VOID_EXPRESSION(bench_session, name, expression) \
    MakeBenchmarkVoidFunction(                                  \
        bench_session,                                             \
        name,                                                      \
        [&]() { expression; })();
