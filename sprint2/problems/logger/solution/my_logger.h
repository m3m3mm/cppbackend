#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <filesystem>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }
        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return ss.str();
    }

    std::string GetLogFilePath() const {
        return "/var/log/sample_log_" + GetFileTimeStamp() + ".log";
    }

    void EnsureFileOpen() {
        std::string current_path = GetLogFilePath();
        if (!log_file_.is_open() || current_path != current_file_path_) {
            if (log_file_.is_open()) {
                log_file_.close();
            }
            // Truncate if file does not exist, otherwise append
            std::ios_base::openmode mode = std::ios::app;
            if (!std::filesystem::exists(current_path)) {
                mode = std::ios::trunc;
            }
            log_file_.open(current_path, mode);
            current_file_path_ = current_path;
        }
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        EnsureFileOpen();
        
        log_file_ << GetTimeStamp() << ": ";
        ((log_file_ << args), ...);
        log_file_ << std::endl;
        log_file_.flush();
    }

    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
        // Force file reopen on timestamp change to ensure proper date handling
        if (log_file_.is_open()) {
            log_file_.close();
            current_file_path_.clear();
        }
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::ofstream log_file_;
    std::string current_file_path_;
    std::mutex mutex_;
};
