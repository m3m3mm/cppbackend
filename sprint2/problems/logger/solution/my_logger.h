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
#include <stdexcept>

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

    void OpenLogFile() {
        std::string filename = "sample_log_" + GetFileTimeStamp() + ".log";
        std::filesystem::create_directories("logs");
        std::string fullpath = "logs/" + filename;
        log_file_.open(fullpath, std::ios::app);
        if (!log_file_.is_open()) {
            throw std::runtime_error("Failed to open log file: " + fullpath);
        }
    }

    Logger() {
        OpenLogFile();
    }

    ~Logger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    static void SetLogDir(const std::string& dir) {
        std::lock_guard<std::mutex> lock(log_dir_mutex_);
        log_dir_ = dir;
    }

    static std::string GetLogDir() {
        std::lock_guard<std::mutex> lock(log_dir_mutex_);
        return log_dir_.empty() ? "/var/log" : log_dir_;
    }

    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if we need to open a new file
        std::string current_date = GetFileTimeStamp();
        if (current_date != last_date_) {
            if (log_file_.is_open()) {
            log_file_.close();
            }
            OpenLogFile();
            last_date_ = current_date;
        }

        // Write timestamp
        log_file_ << GetTimeStamp() << ": ";
        
        // Write all arguments
        ((log_file_ << args), ...);
        
        log_file_ << std::endl;
        log_file_.flush();

        if (log_file_.fail()) {
            throw std::runtime_error("Failed to write to log file");
        }
    }

    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::ofstream log_file_;
    std::mutex mutex_;
    std::string last_date_ = GetFileTimeStamp();
    static std::string log_dir_;
    static std::mutex log_dir_mutex_;
};

// Static members definition
inline std::string Logger::log_dir_ = "";
inline std::mutex Logger::log_dir_mutex_;
