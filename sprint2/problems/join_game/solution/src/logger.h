#pragma once 

#include <boost/date_time.hpp>
#include <boost/json/parse.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>        // для logging::core

#include <string>
#include <string_view>
#include <optional>

namespace logger {
namespace log = boost::log;

void MyFormatter(log::record_view const& rec, log::formatting_ostream& strm);
void InitBoostLogFilter();

void LogExecution(const boost::json::object& val, std::string_view message);
}//namespace logger