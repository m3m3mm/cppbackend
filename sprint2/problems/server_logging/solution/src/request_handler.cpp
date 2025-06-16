#include <iostream>

#include "request_handler.h"

using namespace std::literals;
using namespace targets_storage;

using std::string;
using std::string_view;
using std::vector;

namespace http_handler {
//_________API_HANDLER_________
StringResponse ApiHandler::HanldeApiRequest(const StringRequest &req, targets_storage::TargetRequestType req_type, const model::Game &game){
    using namespace targets_storage; 

    auto start = std::chrono::high_resolution_clock::now();
    http::status status;

    StringResponse responce;

    switch (req_type) {
        case TargetRequestType::GET_MAPS_INFO : {
            status = http::status::ok;
            responce.body() = json_constructor::MakeBodyJSON(req_type, game);
            responce.result(status);
            break;
        }

        case TargetRequestType::GET_MAP_BY_ID : {
            std::string req_obj = ComputeRequestedObject(req.target());
            if(!(game.FindMap(model::Map::Id{req_obj}))) {
                status = http::status::not_found;
                responce.result(status);
            } else {
                status = http::status::ok;
                responce.result(status);
            }
            responce.body() = json_constructor::MakeBodyJSON(req_type, game, req_obj);
            break;
        }

        default : {
            responce.body() = json_constructor::MakeBodyJSON(req_type);
            responce.result(http::status::bad_request);
        }
    }

    responce.version(req.version());
    responce.keep_alive(req.keep_alive());
    responce.insert(http::field::content_type, ContentType::APPLICATION_JSON);
    responce.content_length(responce.body().size());

    auto duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
    logger::LogExecution(json_constructor::MakeLogResponceJSON(duration, static_cast<unsigned>(status), 
                                                               ContentType::APPLICATION_JSON), "response sent");
    return responce;    
}

Strand ApiHandler::GetApiStrand() {
    return api_strand_;
}

std::string ApiHandler::ComputeRequestedObject(std::string_view target) {
    size_t pos = UsingTargetPath::MAPS.size(); 
    char delim = '/';
    return  std::string(target.substr(target.find_first_of('/', pos) + 1 /*чтобы объект был без черты в начале*/));
}

//_________REQUEST_HANDLER_________
constexpr static string_view INDEX_FILE_PATH = "index.html"sv;

BodyResponceVariant RequestHandler::HandleFileRequest(const StringRequest& req, const  fs::path& req_path) {
    auto start = std::chrono::high_resolution_clock::now();

    auto abs_req_path = ComputeReqestedPath(req_path); 
    auto content_type = ""sv;
    http::status status;
    
    BodyResponceVariant result;

    if(IsSubPath(abs_req_path)) {
        
        if(auto file = ComputeExistingFile(abs_req_path)) {
            content_type = ComputeContentType(abs_req_path);
            status = http::status::ok;
            result = MakeFileResponse(req, std::move(file.value()), content_type, status);
        } else {
            content_type = ContentType::TEXT_PLAIN;
            status = http::status::not_found;
            result = MakeStringOtherResponse(status, req, TargetErrorMessage::ERROR_SEARCH_FILE_MESSAGE);
        }
    } else {
        content_type = ContentType::TEXT_PLAIN;   
        status = http::status::bad_request;
        result = MakeStringOtherResponse(status, req, TargetErrorMessage::ERROR_INVALID_PATH_MESSAGE);
    }

    auto duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
    logger::LogExecution(json_constructor::MakeLogResponceJSON(duration, static_cast<unsigned>(status), content_type),
                                                               "response sent");
    return result;
}

FileResponse RequestHandler::MakeFileResponse(const StringRequest& req, http::file_body::value_type&& file, 
                                              string_view content_type, http::status status){
    FileResponse response;

    response.body() = std::move(file);
    response.version(req.version());
    response.result(status);
    response.insert(http::field::content_type, content_type);
    response.prepare_payload();

    return response;
}

StringResponse RequestHandler::MakeStringOtherResponse(http::status status, const StringRequest& req,
                                                       string_view message, string_view content_type) {
    StringResponse response(status, req.version());
    response.set(http::field::content_type, content_type);
    response.body() = message;
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    return response;
}

fs::path RequestHandler::DecodeURL(const fs::path& req_path) {
    string decoded_url = "";
    string encoded_url = req_path.string(); 
    size_t pos = 0;
    
    while (pos < encoded_url.length()) {
        size_t nextPos = encoded_url.find('%', pos);
        if (nextPos == encoded_url.npos) {
            decoded_url += encoded_url.substr(pos);
            break;
        }
        decoded_url += encoded_url.substr(pos, nextPos - pos);
        int hexValue = std::stoi(encoded_url.substr(nextPos + 1, 2), nullptr, 16);
        decoded_url += char(hexValue);
        pos = nextPos + 3;
    }

    return decoded_url;
}

TargetRequestType RequestHandler::ComputeRequestType(string_view target) {
    if(target.starts_with(UsingTargetPath::MAPS)) {
        return target.size() == UsingTargetPath::MAPS.size() ? TargetRequestType::GET_MAPS_INFO
                                                             : TargetRequestType::GET_MAP_BY_ID;
    } else if(!target.starts_with(UsingTargetPath::API)) {
        return TargetRequestType::GET_FILE;
    } else if(target.starts_with(UsingTargetPath::API)) {
        return TargetRequestType::ERROR_API;
    } else {
        return TargetRequestType::UNKNOW;
    }
}

fs::path RequestHandler::ComputeReqestedPath(const fs::path& req_path) {
    auto path = req_path.string().substr(1);
    if(path.empty() || path == "/"sv) {
        path = INDEX_FILE_PATH;
    }
    return fs::weakly_canonical(base_path_/path);
}

string_view RequestHandler::ComputeContentType(const fs::path& req_path) {
    auto extension = req_path.extension().string();

    auto iter = ExtensionToContentType.find(extension);

    if(iter == ExtensionToContentType.end()) {
        return ContentType::APPLICATION_OCTET;
    }

    return iter->second;
}

std::optional<http::file_body::value_type> RequestHandler::ComputeExistingFile(const string& req_path) {
    http::file_body::value_type file;

    if (sys::error_code ec; file.open(req_path.data(), beast::file_mode::read, ec), ec) {
        return std::nullopt;
    }

    return file;
}

bool RequestHandler::IsSubPath(const fs::path& req_path) {
    for (auto b = base_path_.begin(), p = req_path.begin(); b != base_path_.end(); ++b, ++p) {
        if (p == req_path.end() || *p != *b) {
            return false;
        }
    }

    return true;
}

bool RequestHandler::ValidateCompability(http::verb method, TargetRequestType req_type) {
    auto meth = valid_compability.find(method);
    return meth != valid_compability.end() && meth->second.contains(req_type);
}
} // namespace http_handler