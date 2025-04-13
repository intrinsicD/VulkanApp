//
// Created by alex on 4/13/25.
//

#include "Logger.h"
#include <vector> // For sink list

namespace Bcg {
    // Define the static member
    std::shared_ptr<spdlog::logger> Log::s_Logger;

    void Log::Init() {
        // Create sinks: one for console, one for file
        std::vector<spdlog::sink_ptr> sinks;
        // Console Sink (colored)
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        // File Sink (e.g., "logs/app.log") - Ensure "logs" directory exists!
         try {
             sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/VulkanApp.log", true)); // true = truncate
         } catch (const spdlog::spdlog_ex& ex) {
             // Fallback or just use console if file fails
             spdlog::error("Log file creation failed: {}", ex.what());
             // Or just use console sink:
             // sinks.clear();
             // sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
         }


        // Set custom pattern [%Timestamp] [Level] [LoggerName]: Message
        sinks[0]->set_pattern("%^[%T.%e] [%l] %n: %v%$"); // Console pattern with color
        if (sinks.size() > 1) {
            sinks[1]->set_pattern("[%Y-%m-%d %T.%e] [%l] %n: %v"); // File pattern
        }

        // Create a logger with multiple sinks
        s_Logger = std::make_shared<spdlog::logger>("APP", begin(sinks), end(sinks));

        // Register it so spdlog::get() can find it if needed elsewhere
        spdlog::register_logger(s_Logger);

        // Set the logging level (e.g., trace for debug, info for release)
#ifdef NDEBUG // Release mode
        s_Logger->set_level(spdlog::level::info);
        s_Logger->flush_on(spdlog::level::info); // Flush on info level and above
#else // Debug mode
        s_Logger->set_level(spdlog::level::trace);
        s_Logger->flush_on(spdlog::level::trace); // Flush immediately in debug
#endif
        // Set spdlog's default logger in case other libraries use it indirectly
        spdlog::set_default_logger(s_Logger);

         Info("Logging Initialized."); // Use the logger
    }
} // namespace Bcg