#include "api_handler.h"
#include "../server/logger.h"

using namespace app;
using namespace extra_data;
using namespace json_constructor;
using namespace std::literals;
using namespace targets_storage;
using namespace model;

using std::string;
using std::string_view;

namespace http_handler {
const static string NO_CACHE = "no-cache";
const static string RESPONSE_SENT = "response sent";

//_________Ticker_________
Ticker::Ticker(Strand& strand, std::chrono::milliseconds&& period, Handler handler)
    : strand_(strand)
    , period_(std::forward<std::chrono::milliseconds>(period))
    , handler_(handler) {
}

void Ticker::Start() {
    last_tick_ = std::chrono::steady_clock::now();
    ScheduleTick();
}

void Ticker::ScheduleTick() {
    timer_.expires_after(std::chrono::duration_cast<std::chrono::milliseconds>(period_));
    timer_.async_wait(
        net::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
    }));
}

void Ticker::OnTick(sys::error_code ec) {
    auto current_tick = std::chrono::steady_clock::now();
    handler_(std::chrono::duration_cast<std::chrono::milliseconds>(current_tick - last_tick_));
    last_tick_ = current_tick;
    ScheduleTick();
}

//_________BuilderApiHandler_________
BuilderApiHandler& BuilderApiHandler::SetStrand(Strand&& strand) {
    api_strand_ = std::make_unique<Strand>(std::move(strand));
    return *this;
}

BuilderApiHandler& BuilderApiHandler::SetGame(Game&& game) {
    game_ = std::make_unique<Game>(std::move(game));
    return *this;
}

BuilderApiHandler& BuilderApiHandler::SetLootTypes(LootTypes&& loot_types) {
    loot_types_ = std::make_unique<LootTypes>(std::move(loot_types));
    return *this;
}

BuilderApiHandler& BuilderApiHandler::SetTimer(std::chrono::milliseconds&& timer) {
    timer_ = std::make_unique<std::chrono::milliseconds>(std::move(timer));
    return *this; 
}

http_handler::ApiHandler BuilderApiHandler::Build() {
    return ApiHandler(std::move(*api_strand_.release()),
                      std::move(*game_.release()),
                      std::move(*loot_types_.release()),
                      std::move(*timer_.release()));
}

//_________ApiHandler_________
ApiHandler::ApiHandler(Strand&& api_strand, model::Game&& game, extra_data::LootTypes&& loot_types, std::chrono::milliseconds&& timer) 
    : api_strand_(std::forward<Strand>(api_strand))
    , loot_types_(std::forward<extra_data::LootTypes>(loot_types))
    , app_(std::forward<model::Game>(game))
    , timer_(std::forward<std::chrono::milliseconds>(timer)) {

}

void ApiHandler::Start() {
    if(timer_.count() != 0) {
        ticker_ = std::make_shared<Ticker>(api_strand_, 
                                           std::forward<std::chrono::milliseconds>(timer_), [&](std::chrono::milliseconds delta) {
                                           app_.ProcessTickActions(delta);
                                           });
        ticker_->Start(); 
    }
}

StringResponse ApiHandler::HanldeApiRequest(const StringRequest& req, TargetRequestType req_type) {
    auto start = std::chrono::high_resolution_clock::now();

    http::status status;
    StringResponse response;

    std::unique_ptr<app::ResponseInfo> resp_info = nullptr;

    switch (req_type) {
        case TargetRequestType::GET_MAPS_INFO :
        case TargetRequestType::GET_MAP_BY_ID : {
            string req_obj = ComputeRequestedObject(req.target());
            resp_info = std::make_unique<ResponseInfo>(app_.GetStaticObjectsInfo(req_type, req_obj, loot_types_));
            break;
        }

        case TargetRequestType::GET_PLAYERS :
        case TargetRequestType::GET_STATE   : 
            resp_info = std::make_unique<ResponseInfo>(app_.GetPlayersReqInfo(req_type, req.base()));
            break;
        
        case TargetRequestType::POST_JOIN_GAME : 
            resp_info = std::make_unique<ResponseInfo>(app_.JoinGame(req.body()));
            break;
        
        case TargetRequestType::POST_ACTION : 
            resp_info = std::make_unique<ResponseInfo>(app_.UpdateState(req.base(), req.body()));
            break;
        
        case TargetRequestType::POST_TICK : {
            if(ticker_.get() == nullptr) {
                resp_info = std::make_unique<ResponseInfo>(app_.ProcessTickActions(req.base(), req.body()));
                break;
            }
        }

        default : {
            status = http::status::bad_request;
            response.body() = MakeBodyErrorJSON(TargetErrorCode::ERROR_BAD_REQUEST_CODE,
                                                TargetErrorMessage::ERROR_BAD_REQUEST_MESSAGE);
            break;
        }
    }

    if(resp_info) {
        status = resp_info->status;
        response.body() = resp_info->body;
    }
    response.insert(http::field::cache_control, NO_CACHE);
    response.result(status);
    response.version(req.version());
    response.keep_alive(req.keep_alive());
    response.insert(http::field::content_type, ContentType::APPLICATION_JSON);
    response.content_length(response.body().size());

    auto duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
    logger::LogExecution(MakeLogResponceJSON(duration, static_cast<unsigned>(status), 
                                             ContentType::APPLICATION_JSON), RESPONSE_SENT);

    return response;
}

Strand ApiHandler::GetApiStrand() const {
    return api_strand_;
}

string ApiHandler::ComputeRequestedObject(string_view target) const {
    size_t pos = UsingTargetPath::MAPS.size(); 
    char delim = '/';
    return pos == target.size() ? ""s
                                : string(target.substr(target.find_first_of(delim, pos) + 1));
}
}