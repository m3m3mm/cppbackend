#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/object.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "model.h"

RequestInfo ComputeRequestInfo(std::string_view request);

std::string MakeResponceJSON(bool is_success);
std::string MakeResponceJSON(const std::vector<Book>& books);