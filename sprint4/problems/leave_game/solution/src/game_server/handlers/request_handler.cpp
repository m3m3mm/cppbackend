#include <charconv>
#include <regex>
#include <utility>

#include "../json/json_loader.h"
#include "request_handler.h"

namespace sys = boost::system;

using namespace app;
using namespace json_constructor;
using namespace std::literals;
using namespace targets_storage;

using std::string;
using std::string_view;

namespace http_handler {
const static string_view NO_CACHE = "no-cache";
const static string_view RESPONSE_SENT = "response sent";
constexpr static string_view INDEX_FILE_PATH = "index.html"sv;

//_________ReqestHandler_________
RequestHandler::RequestHandler(fs::path&& base_path, ApiHandler&& api_handler, int save_period)
    : base_path_(fs::weakly_canonical(base_path))
    , api_handler_(std::forward<ApiHandler>(api_handler)) {
    api_handler_.Start(std::chrono::milliseconds(save_period));
}

void RequestHandler::SaveState() const {
    api_handler_.SaveState();
}

BodyResponceVariant RequestHandler::HandleFileRequest(const StringRequest& req, const fs::path& req_path) {
    auto start = std::chrono::high_resolution_clock::now();

    auto abs_req_path = ComputeReqestedPath(req_path); 
    auto content_type = ""sv;
    http::status status;
    
    BodyResponceVariant response;

    if(IsSubPath(abs_req_path)) {
        
        if(auto file = ComputeExistingFile(abs_req_path)) {
            content_type = ComputeContentType(abs_req_path);
            status = http::status::ok;
            response = MakeFileResponse(req, std::move(file.value()), content_type, status);
        } else {
            content_type = ContentType::TEXT_PLAIN;
            status = http::status::not_found;
            response = MakeStringOtherResponse(status, req, TargetErrorMessage::ERROR_SEARCH_FILE_MESSAGE);
        }
    } else {
        content_type = ContentType::TEXT_PLAIN;   
        status = http::status::bad_request;
        response = MakeStringOtherResponse(status, req, TargetErrorMessage::ERROR_INVALID_PATH_MESSAGE);
    }

    auto duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
    logger::LogExecution(MakeLogResponceJSON(duration, static_cast<unsigned>(status), content_type),
                                             RESPONSE_SENT);
    return response;
}

StringResponse RequestHandler::HandleErrorRequest(const StringRequest& req, TargetRequestType req_type) {
    auto start = std::chrono::high_resolution_clock::now();
    http::status status;

    StringResponse response;
    string content_type = "";

    switch (req_type) {
        case TargetRequestType::UNKNOW : {
            status = http::status::bad_request;
            content_type = ContentType::TEXT_PLAIN;
            response = MakeStringOtherResponse(status, req, TargetErrorMessage::ERROR_INVALID_METHOD_MESSAGE);
            break;
        }

        default: {
            auto message = MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_METHOD_CODE,
                                             TargetErrorMessage::ERROR_INVALID_METHOD_MESSAGE);
            status = http::status::method_not_allowed;
            content_type = ContentType::APPLICATION_JSON;
            string target = "";
            if(req.target().starts_with(UsingTargetPath::MAPS)){
                target = UsingTargetPath::MAPS;
            } else {
                target = req.target();
            }
            
            response = MakeStringOtherResponse(status, req, message, content_type,
                                               &allowed_methods.find(target)->second);
            break;
        }
    }
    response.result(status);

    auto duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
    logger::LogExecution(MakeLogResponceJSON(duration, static_cast<unsigned>(status), content_type),
                                             RESPONSE_SENT);
    return response;                                                          
}

FileResponse RequestHandler::MakeFileResponse(const StringRequest& req, http::file_body::value_type&& file, 
                                              string_view content_type, http::status status) {
    FileResponse response; 

    response.body() = std::move(file);
    response.version(req.version());
    response.result(status);
    response.insert(http::field::content_type, content_type);
    response.prepare_payload();

    return response;
}

StringResponse RequestHandler::MakeStringOtherResponse(http::status status, 
                                                       const StringRequest& req, 
                                                       string_view message, 
                                                       string_view content_type, 
                                                       const std::vector<http::verb>* allowed_methods) {
    StringResponse response(status, req.version());

    response.set(http::field::content_type, content_type);
    response.body() = message;
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    response.insert(http::field::cache_control, NO_CACHE);

    if(allowed_methods && !allowed_methods->empty()) {
        for(const auto& method : *allowed_methods) {
            response.insert(http::field::allow, http::to_string(method));
        } 
    }

    return response;
}

const static int VALUE_SIZE = 2;

fs::path RequestHandler::DecodeURL(const fs::path& req_path) const {
    string decoded_url = "";
    string encoded_url = req_path.string();
    size_t pos = 0;
    
    while (pos < encoded_url.length()) {
        if(size_t next_pos; (next_pos = encoded_url.find('%', pos)) != encoded_url.npos
                         || (next_pos = encoded_url.find('+', pos)) != encoded_url.npos) {
                            
            if(encoded_url[next_pos] == '%'){
                decoded_url += encoded_url.substr(pos, next_pos - pos);

                std::string value = encoded_url.substr(next_pos + 1, VALUE_SIZE);
                if(value.size() < VALUE_SIZE) {
                    throw std::invalid_argument("Encoded character must be of the form %XY");
                }

                int number = 0;
                const auto [ptr, ec] = std::from_chars(value.begin().base(), value.end().base(), number, 16);
                if(ec != std::errc() || ptr - value.begin().base() != VALUE_SIZE) {
                     throw std::invalid_argument("Encoded character must contain only hexadecimal values");

                }
                decoded_url += char(number);
                pos = next_pos + 3;
            }

            if(encoded_url[next_pos] == '+') {
                decoded_url += encoded_url.substr(pos, next_pos - pos);
                decoded_url += " ";
                pos = next_pos + 1;
            }
        } else {
            decoded_url += encoded_url.substr(pos);
            break;
        }
    }

    return decoded_url;
}

TargetRequestType RequestHandler::ComputeRequestType(string_view target) const {
    if(target.starts_with(UsingTargetPath::MAPS)) {
        return target.size() == UsingTargetPath::MAPS.size() ? TargetRequestType::GET_MAPS_INFO
                                                             : TargetRequestType::GET_MAP_BY_ID;
    } else if(target.starts_with(UsingTargetPath::RECORDS)){
        return TargetRequestType::GET_RECORDS;
    }else if(target.starts_with(UsingTargetPath::TICK)){
        return TargetRequestType::POST_TICK;
    }else if(target.starts_with(UsingTargetPath::ACTION)) {
        return TargetRequestType::POST_ACTION;
    } else if(target.starts_with(UsingTargetPath::STATE)) {
        return TargetRequestType::GET_STATE;
    } else if (target.starts_with(UsingTargetPath::PLAYERS)) {
        return TargetRequestType::GET_PLAYERS;
    } else if (target.starts_with(UsingTargetPath::JOIN)) {
        return TargetRequestType::POST_JOIN_GAME;
    }else if(!target.starts_with(UsingTargetPath::API)) {
        return TargetRequestType::GET_FILE;
    } else if(target.starts_with(UsingTargetPath::API)) {
        return TargetRequestType::ERROR_API;
    }
        
    return TargetRequestType::UNKNOW;
}

fs::path RequestHandler::ComputeReqestedPath(const fs::path& req_path) const {
    auto path = req_path.string().substr(1);
    if(path.empty() || path == "/"sv) {
        path = INDEX_FILE_PATH;
    }
    return fs::weakly_canonical(base_path_/path);
}

string_view RequestHandler::ComputeContentType(const fs::path& req_path) const {
    auto extension = req_path.extension().string();

    auto iter = ExtensionToContentType.find(extension);

    if(iter == ExtensionToContentType.end()) {
        return ContentType::APPLICATION_OCTET;
    }

    return iter->second;
}

std::optional<http::file_body::value_type> RequestHandler::ComputeExistingFile(const string& req_path) const {
    http::file_body::value_type file;

    if (sys::error_code ec; file.open(req_path.data(), beast::file_mode::read, ec), ec) {
        return std::nullopt;
    }

    return file;
}

bool RequestHandler::IsSubPath(const fs::path& req_path) const {
    for (auto b = base_path_.begin(), p = req_path.begin(); b != base_path_.end(); ++b, ++p) {
        if (p == req_path.end() || *p != *b) {
            return false;
        }
    }

    return true;
}

bool RequestHandler::ValidateCompability(http::verb method, TargetRequestType req_type) const {
    auto meth = valid_compability.find(method);
    return meth != valid_compability.end() && meth->second.contains(req_type);
}
} // namespace http_handler