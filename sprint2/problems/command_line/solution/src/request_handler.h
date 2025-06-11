#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "application.h"
#include "http_server.h"
#include "json_constructor.h"
#include "logger.h"
#include "target_storage.h"

namespace http_handler {
namespace beast = boost::beast;
namespace fs = std::filesystem;
namespace http = beast::http;
namespace net = boost::asio;
namespace sys = boost::system;

using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;

using BodyResponceVariant = std::variant<StringResponse, FileResponse>;
using Strand = net::strand<net::io_context::executor_type>;
using Header = http::header<true, beast::http::fields>;

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Handler = std::function<void(std::chrono::milliseconds)>;

    Ticker(Strand& strand, std::chrono::milliseconds&& period, Handler handler);

    void Start();
private:
    void ScheduleTick();
    void OnTick(sys::error_code ec);

private:
    Strand& strand_;
    net::steady_timer timer_{strand_};
    std::chrono::milliseconds period_;
    Handler handler_;
    std::chrono::time_point<std::chrono::steady_clock> last_tick_;
}; 

class ApiHandler {
public:
    ApiHandler(Strand&& api_strand, model::Game&& game, std::chrono::milliseconds&& timer);
    
    StringResponse HanldeApiRequest(const StringRequest& req, targets_storage::TargetRequestType req_type);
    Strand GetApiStrand() const;
private:
    Strand api_strand_;
    app::Application app_;
    std::shared_ptr<Ticker> ticker_;

    std::string ComputeRequestedObject(std::string_view target) const;
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    explicit RequestHandler(Strand&& api_strand, model::Game&& game, 
                            fs::path&& base_path, std::chrono::milliseconds&& timer);
                            
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        using namespace targets_storage;
    
        auto decoded_req_path = DecodeURL(req.target()).string();                                 
        auto req_type = ComputeRequestType(decoded_req_path);

        auto iter = decoded_req_path.find_last_of("."s);

        if(iter != decoded_req_path.npos) {
            std::transform(decoded_req_path.begin() + iter + 1, 
                           decoded_req_path.end(), 
                           decoded_req_path.begin() + iter + 1,
                           [](unsigned char c){ 
                               return std::tolower(c); 
                           });
        }

        if(ValidateCompability(req.method(), req_type)) {
            switch (req_type) {
                case TargetRequestType::GET_FILE : {
                    std::visit(
                        [&send](auto&& result) {
                            send(std::forward<decltype(result)>(result));
                        },
                        HandleFileRequest(req, decoded_req_path));
                    break;
                }
                
                //Будем считать, что по умолчанию /api
                default : {
                    auto handle = [self = shared_from_this(), send,
                                   req = std::forward<decltype(req)>(req), req_type] {
                        assert(self->api_handler_.GetApiStrand().running_in_this_thread());
                        return send(self->api_handler_.HanldeApiRequest(req, req_type));
                    };

                    net::dispatch(api_handler_.GetApiStrand(), handle);
                    break;
                }
            }
        } else {
            send(HandleErrorRequest(req, req_type));
        }
    }

private:
    fs::path base_path_;
    ApiHandler api_handler_;

    BodyResponceVariant HandleFileRequest(const StringRequest& req, const fs::path& req_path);
    StringResponse HandleErrorRequest(const StringRequest& req, targets_storage::TargetRequestType req_type);

    FileResponse MakeFileResponse(const StringRequest& req, http::file_body::value_type&& file, std::string_view content_type,
                                  http::status status);

    StringResponse MakeStringOtherResponse(http::status status, const StringRequest& req,
                                           std::string_view message,
                                           std::string_view content_type = targets_storage::ContentType::TEXT_PLAIN,
                                           std::string_view allowed_methods = "");

    fs::path DecodeURL(const fs::path& req_path) const;

    targets_storage::TargetRequestType ComputeRequestType(std::string_view target) const;
    fs::path ComputeReqestedPath(const fs::path& req_path) const;
    std::string_view ComputeContentType(const fs::path& req_path) const;

    //Возвращает nullopt, если файл не удалось открыть, либо он не существует
    std::optional<http::file_body::value_type> ComputeExistingFile(const std::string& req_path) const;
    
    bool IsSubPath(const fs::path& req_path) const;
    bool ValidateCompability(http::verb method, targets_storage::TargetRequestType req_type) const;
};
}  // namespace http_handler