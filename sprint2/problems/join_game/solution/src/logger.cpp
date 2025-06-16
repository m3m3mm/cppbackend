#include <boost/json/serialize.hpp>
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр 
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include "logger.h"

namespace logger {
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace log = boost::log;
namespace sinks = boost::log::sinks;

using namespace std::literals;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(json_data, "JsonData", boost::json::object);

const static std::string TIMESTAMP = "timestamp"s;
const static std::string DATA = "data"s;
const static std::string MESSAGE = "message"s;

void MyFormatter(log::record_view const &rec, log::formatting_ostream &strm) {
    boost::json::object val;
    
    val[TIMESTAMP] = to_iso_extended_string(*rec[timestamp]);
    val[DATA] = *rec[json_data];
    val[MESSAGE] = *rec[expr::smessage];

    strm << boost::json::serialize(val);
}

void InitBoostLogFilter() {
    log::add_common_attributes();

    log::add_console_log(
        std::cout,
        keywords::auto_flush = true,
        keywords::format = &MyFormatter
    );
}

void LogExecution(const boost::json::object& val, std::string_view message) {
    BOOST_LOG_TRIVIAL(info) << log::add_value(json_data, val) << message;
}
}//namespace logger