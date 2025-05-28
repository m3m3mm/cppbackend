#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/json.hpp>
#include <string>
#include <string_view>

namespace logging {

namespace json = boost::json;
using namespace std::literals;

// Define attribute keywords
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

// Initialize logging
void InitLogging();

// Log server start
void LogServerStart(unsigned short port, std::string_view address);

// Log server stop
void LogServerStop(int code, const std::exception* ex = nullptr);

// Log request
void LogRequest(std::string_view remote_addr, std::string_view uri, std::string_view method);

// Log response
void LogResponse(std::string_view remote_addr, int response_time, int response_code, std::string_view content_type);

// Log error
void LogError(int code, std::string_view text, std::string_view where);

} // namespace logging 