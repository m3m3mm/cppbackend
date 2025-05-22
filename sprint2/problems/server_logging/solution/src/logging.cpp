#include "logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/json.hpp>

namespace logging {

void InitLogging() {
    boost::log::add_common_attributes();
}

void LogRequest(std::string_view remote_addr, std::string_view uri, std::string_view method) {
    boost::json::object request;
    request["remote_addr"] = std::string(remote_addr);
    request["uri"] = std::string(uri);
    request["method"] = std::string(method);
    
    BOOST_LOG_TRIVIAL(info) << "request: " << boost::json::serialize(request);
}

void LogResponse(std::string_view remote_addr, int response_time, int response_code, std::string_view content_type) {
    boost::json::object response;
    response["remote_addr"] = std::string(remote_addr);
    response["response_time"] = response_time;
    response["response_code"] = response_code;
    response["content_type"] = std::string(content_type);
    
    BOOST_LOG_TRIVIAL(info) << "response: " << boost::json::serialize(response);
}

void LogServerStart(unsigned short port, std::string_view address) {
    boost::json::object server;
    server["port"] = port;
    server["address"] = std::string(address);
    
    BOOST_LOG_TRIVIAL(info) << "server_start: " << boost::json::serialize(server);
}

void LogServerStop(int code, const std::exception* ex) {
    boost::json::object data;
    data["code"] = code;
    if (ex) {
        data["exception"] = ex->what();
    }
    
    BOOST_LOG_TRIVIAL(info) << "server_stop: " << boost::json::serialize(data);
}

void LogError(int code, std::string_view text, std::string_view where) {
    boost::json::object error;
    error["code"] = code;
    error["text"] = std::string(text);
    error["where"] = std::string(where);
    
    BOOST_LOG_TRIVIAL(error) << "error: " << boost::json::serialize(error);
}

} // namespace logging 