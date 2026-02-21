/*
 * Copyright (c) 2026 SeolYang(Yang Kyowon)
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once
#include <Mon3tr/Core/Assertion.hpp>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#pragma warning(push)
#pragma warning(disable : 26800)
#pragma warning(disable : 26498)
#pragma warning(disable : 26437)
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#pragma warning(pop)

namespace mon3tr {
    enum class ELogVerbosity {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    class LogSystem final {
    public:
        ~LogSystem() = default;

        LogSystem(const LogSystem&) = delete;

        LogSystem(LogSystem&&) noexcept = delete;

        LogSystem& operator=(const LogSystem&) = delete;

        LogSystem& operator=(LogSystem&&) noexcept = delete;

        static LogSystem& GetInstance() {
            static LogSystem instance{};
            return instance;
        }

        std::unique_ptr<spdlog::logger> CreateLogger(const std::string_view category) {
            spdlog::logger* newLogger = new spdlog::logger{category.data(), spdlog::sinks_init_list{consoleSink_, fileSink_}};
            newLogger->set_level(spdlog::level::trace);
            return std::unique_ptr<spdlog::logger>{newLogger};
        }

    private:
        LogSystem() : consoleSink_(std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::always))
                    , fileSink_(std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt", true)) {
            constexpr std::string_view kFormatHeaderPattern = "[%Y-%m-%d %H:%M:%S] [%n]  %^%v%$";
            consoleSink_->set_pattern(kFormatHeaderPattern.data());
            fileSink_->set_pattern(kFormatHeaderPattern.data());
        }

    private:
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> consoleSink_;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt>   fileSink_;
    };
}

/**
 * @brief 로그 시스템에 등록할 카테고리 이름을 선언합니다.
 * @warning 글로벌 네임스페이스에 카테고리 선언 매크로가 위치하여야 합니다.
 * @param CATEGORY 선언할 카테고리의 이름
 */
#define M3_DECLARE_LOG_CATEGORY(CATEGORY) \
namespace mon3tr::internal::log::CATEGORY { \
    extern spdlog::logger* GetLogger(); \
    template<ELogVerbosity Verbosity, typename... Args> \
    void Log(const std::string_view message, Args&&... args) { \
        spdlog::logger* logger = GetLogger(); \
        M3_ASSERT(logger != nullptr); \
        const std::string formattedMessage = std::vformat(message, std::make_format_args(args...)); \
        if constexpr (Verbosity == ELogVerbosity::Debug) { \
            logger->debug(formattedMessage); \
        } \
        else if constexpr (Verbosity == ELogVerbosity::Info) { \
            logger->info(formattedMessage); \
        } \
        else if constexpr (Verbosity == ELogVerbosity::Warning) { \
            logger->warn(formattedMessage); \
        } \
        else if constexpr (Verbosity == ELogVerbosity::Error) { \
            logger->error(formattedMessage); \
        } \
        else if constexpr (Verbosity == ELogVerbosity::Fatal) { \
            logger->critical(formattedMessage); \
        } \
        else { \
            logger->trace(formattedMessage); \
        } \
    } \
}

#define M3_DEFINE_LOG_CATEGORY(CATEGORY) \
namespace mon3tr::internal::log::CATEGORY { \
    spdlog::logger* GetLogger() { \
        static std::unique_ptr<spdlog::logger> logger{LogSystem::GetInstance().CreateLogger(#CATEGORY) }; \
        return logger.get(); \
    } \
}

#define M3_LOG(CATEGORY, VERBOSITY, MESSAGE, ...) \
mon3tr::internal::log::CATEGORY::Log<mon3tr::ELogVerbosity::VERBOSITY>(MESSAGE, __VA_OPT__(,) __VA_ARGS__);
