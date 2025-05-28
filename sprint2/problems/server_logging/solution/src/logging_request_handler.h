#pragma once

#include "request_handler.h"
#include "logging.h"
#include <chrono>
#include <string_view>
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

template<typename RequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(RequestHandler& handler) 
        : decorated_(handler) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Log request
        logging::LogRequest(
            "-",  // placeholder for remote address
            std::string(req.target()),
            std::string(req.method_string())
        );

        // Forward the request to the decorated handler with a wrapped send callback
        decorated_(std::move(req), 
            [send = std::forward<Send>(send), start_time](auto&& response) mutable {
                // Calculate response time
                auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();

                // Get content type
                std::string content_type;
                if (auto it = response.find(http::field::content_type); it != response.end()) {
                    content_type = std::string(it->value());
                }

                // Log response
                logging::LogResponse(
                    "-",  // placeholder for remote address
                    static_cast<int>(response_time),
                    response.result_int(),
                    content_type
                );

                // Forward the response
                send(std::forward<decltype(response)>(response));
            });
    }

private:
    RequestHandler& decorated_;
}; 