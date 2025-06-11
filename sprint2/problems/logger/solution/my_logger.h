#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

const static std::string SIMPLE_LOG = "sample_log_";
const static std::string FILE_EXTENSION = ".log";
const static std::string LOG_DIR = "/var/log";

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
        return std::put_time(std::gmtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        std::ostringstream buf;
        buf << GetTimeStamp();
        auto ts = buf.str();

        auto res = ts.substr(0, ts.find_first_of(" "));
        std::replace(res.begin(), res.end(), '-', '_');
        
        return res;
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> g(m_);

        std::filesystem::path log_name = SIMPLE_LOG + GetFileTimeStamp() + FILE_EXTENSION;
        std::filesystem::path log_full_path = std::filesystem::weakly_canonical(LOG_DIR/log_name);

        log_file_.open(log_full_path, std::ios::app);
        log_file_ << GetTimeStamp() << ": ";
        ((log_file_ << std::forward<decltype(args)>(args)), ...); 
        log_file_ << '\n';
        log_file_.close();
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> g(m_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::mutex m_;
    std::ofstream log_file_;
};
