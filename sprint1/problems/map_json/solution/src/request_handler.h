#pragma once
#include "http_server.h"
#include "model.h"
#include <functional>
#include <boost/json.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        req_ = std::move(req);
        send_ = send;  // Copy the callback instead of moving it
        
        const auto& target = req_.target();
        
        // Handle API endpoints
        static const beast::string_view api_maps = "/api/v1/maps";
        static const beast::string_view api_maps_slash = "/api/v1/maps/";
        
        if (target.starts_with("/api/v1/maps")) {
            if (target == "/api/v1/maps") {
                if (req_.method() == http::verb::get) {
                    HandleGetMaps();
                } else {
                    send_(MakeErrorResponse(http::status::method_not_allowed, "methodNotAllowed", "Method not allowed"));
                }
            } else if (target.starts_with("/api/v1/maps/")) {
                if (req_.method() == http::verb::get) {
                    auto id = target.substr(api_maps_slash.length());
                    if (id.empty()) {
                        send_(MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request"));
                    } else {
                        HandleGetMap(id);
                    }
                } else {
                    send_(MakeErrorResponse(http::status::method_not_allowed, "methodNotAllowed", "Method not allowed"));
                }
            } else {
                send_(MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request"));
            }
        } else {
            send_(MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request"));
        }
    }

private:
    void HandleGetMaps();
    void HandleGetMap(beast::string_view id);

    http::response<http::string_body> MakeErrorResponse(http::status status, beast::string_view code, beast::string_view message);
    http::response<http::string_body> MakeSuccessResponse(http::status status, const json::value& body);

    model::Game& game_;
    http::request<http::string_body> req_;
    std::function<void(http::response<http::string_body>&&)> send_;
};

}  // namespace http_handler
