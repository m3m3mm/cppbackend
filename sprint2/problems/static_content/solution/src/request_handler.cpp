#include "request_handler.h"

#include <iostream>

using namespace std::literals;
using namespace targets_storage;

using std::string;
using std::string_view;
using std::vector;

namespace http_handler {
constexpr static string_view INDEX_FILE_PATH = "index.html"sv;

BodyResponceVariant RequestHandler::HandleRequest(StringRequest&& req) {
    switch (req.method()) {
        case http::verb::get : 
        case http::verb::head : 
            return MakeResponse(req);
            break;
        default:
            return MakeStringOtherResponse(http::status::method_not_allowed, req,
                                                TargetErrorMessage::ERROR_INVALID_METHOD_MESSAGE);
            break;
    }
}

BodyResponceVariant RequestHandler::MakeResponse(const StringRequest& req) {  
    auto decoded_req_path = DecodeURL(req.target()).string();                                 
    auto req_type = ComputeRequestType(decoded_req_path);

    auto iter = decoded_req_path.find_last_of("."s);

    std::transform(decoded_req_path.begin() + iter + 1, decoded_req_path.end(), decoded_req_path.begin() + iter + 1,
        [](unsigned char c){ return std::tolower(c); });

    switch (req_type) {
        case TargetRequestType::GET_MAPS_INFO :
        case TargetRequestType::GET_MAP_BY_ID :
        case TargetRequestType::ERROR_API :
            return MakeStringResponse(req_type, req);
            break;

        case TargetRequestType::GET_FILE : {
            auto abs_req_path = ComputeReqestedPath(decoded_req_path);    
            if(IsSubPath(abs_req_path)) {
                
                if(auto file = ComputeExistingFile(abs_req_path)) {
                    return MakeFileResponse(req_type, req, std::move(file.value()), ComputeContentType(abs_req_path));
                } else {
                    return MakeStringOtherResponse(http::status::not_found, req,
                                                   targets_storage::TargetErrorMessage::ERROR_SEARCH_FILE_MESSAGE);
                }
            } else {   
                return MakeStringOtherResponse(http::status::bad_request, req,
                                               targets_storage::TargetErrorMessage::ERROR_INVALID_PATH_MESSAGE);
            }
            break;
        }

        case TargetRequestType::UNKNOW : {
            return MakeStringOtherResponse(http::status::bad_request, req, 
                                           targets_storage::TargetErrorMessage::ERROR_INVALID_PATH_MESSAGE);
            break;
        }         
    } 
}

StringResponse RequestHandler::MakeStringResponse(targets_storage::TargetRequestType req_type, const StringRequest& req) {
    StringResponse responce;

    switch (req_type) {
        case TargetRequestType::GET_MAPS_INFO :
            responce.body() = body::MakeBodyJSON(req_type, game_);
            responce.result(http::status::ok);
            break;

        case TargetRequestType::GET_MAP_BY_ID :{
            std::string req_obj = ComputeRequestedObject(req.target());

            if(!(game_.FindMap(model::Map::Id{req_obj}))) {
                responce.result(http::status::not_found);
            } else {
                responce.result(http::status::ok);
            }

            responce.body() = body::MakeBodyJSON(req_type, game_, req_obj);
            break;
        }

        case TargetRequestType::ERROR_API : {
            responce.body() = body::MakeBodyJSON(req_type);
            responce.result(http::status::bad_request);
        }
            
    }

    responce.version(req.version());
    responce.keep_alive(req.keep_alive());
    responce.insert(http::field::content_type, ContentType::APPLICATION_JSON);
    responce.content_length(responce.body().size());

    return responce;
}

FileResponse RequestHandler::MakeFileResponse(TargetRequestType req_type, const StringRequest &req, 
                                              http::file_body::value_type&& file, string_view content_type) {
    FileResponse responce;

    switch (req_type) {
        case TargetRequestType::GET_FILE :
            responce.body() = std::move(file);
            break;
    }

    responce.version(req.version());
    responce.result(http::status::ok);
    responce.insert(http::field::content_type, content_type);
    responce.prepare_payload();

    return responce;
}

StringResponse RequestHandler::MakeStringOtherResponse(http::status status, const StringRequest &req,
                                                       string_view message, string_view content_type) {
    StringResponse response(status, req.version());
    response.set(http::field::content_type, content_type);
    response.body() = message;
    response.content_length(response.body().size());
    response.keep_alive(req.keep_alive());
    return response;
}

fs::path RequestHandler::DecodeURL(const fs::path &req_path) {
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

string RequestHandler::ComputeRequestedObject(string_view target) {
    size_t pos = UsingTargetPath::MAPS.size(); 
    return  string(target.substr(target.find_first_of('/', pos) + 1 /*чтобы объект был без черты в начале*/));
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
} // namespace http_handler