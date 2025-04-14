//
// Created by alex on 4/13/25.
//

#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h> // For console logging
#include <spdlog/sinks/basic_file_sink.h>   // For file logging
#include <memory>

namespace Bcg {
    class Log {
    public:
        static void Init(); // Call once at startup

        static void setLevel(spdlog::level::level_enum level) {
            s_Logger->set_level(level);
        }

        // Template functions for different levels
        template<typename... Args>
        static void Trace(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            s_Logger->trace(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Debug(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            s_Logger->debug(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Info(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            s_Logger->info(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Warn(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            s_Logger->warn(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Error(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            s_Logger->error(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Critical(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            s_Logger->critical(fmt, std::forward<Args>(args)...);
        }

    private:
        static std::shared_ptr<spdlog::logger> s_Logger;
    };
} // namespace Bcg

#endif //LOGGER_H
