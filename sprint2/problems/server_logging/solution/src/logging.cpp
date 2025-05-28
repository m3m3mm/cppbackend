#include "logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/json.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace logging {

void InitLogging() {
    boost::log::add_common_attributes();
    
    // Remove any existing sinks
    boost::log::core::get()->remove_all_sinks();
    
    // Add a console sink that only outputs the message
    auto console_sink = boost::log::add_console_log();
    console_sink->set_formatter(
        boost::log::expressions::stream << boost::log::expressions::smessage
    );
}

void LogRequest(std::string_view remote_addr, std::string_view uri, std::string_view method) {
    boost::json::object data;
    data["method"] = std::string(method);
    data["URI"] = std::string(uri);
    
    boost::json::object log;
    log["message"] = "request received";
    log["data"] = data;
    
    BOOST_LOG_TRIVIAL(info) << boost::json::serialize(log);
}

void LogResponse(std::string_view remote_addr, int response_time, int response_code, std::string_view content_type) {
    boost::json::object data;
    data["code"] = response_code;
    data["content_type"] = std::string(content_type);
    
    boost::json::object log;
    log["message"] = "response sent";
    log["data"] = data;
    
    BOOST_LOG_TRIVIAL(info) << boost::json::serialize(log);
}

void LogServerStart(unsigned short port, std::string_view address) {
    boost::json::object data;
    data["port"] = port;
    data["address"] = std::string(address);
    
    boost::json::object log;
    log["message"] = "server started";
    log["data"] = data;
    
    BOOST_LOG_TRIVIAL(info) << boost::json::serialize(log);
}

void LogServerStop(int code, const std::exception* ex) {
    boost::json::object data;
    data["code"] = code;
    if (ex) {
        data["exception"] = ex->what();
    }
    
    boost::json::object log;
    log["message"] = "server stopped";
    log["data"] = data;
    
    BOOST_LOG_TRIVIAL(info) << boost::json::serialize(log);
}

void LogError(int code, std::string_view text, std::string_view where) {
    boost::json::object data;
    data["code"] = code;
    data["text"] = std::string(text);
    data["where"] = std::string(where);
    
    boost::json::object log;
    log["message"] = "error occurred";
    log["data"] = data;
    
    BOOST_LOG_TRIVIAL(error) << boost::json::serialize(log);
}

} // namespace logging 