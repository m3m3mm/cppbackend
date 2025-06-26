#include <memory>

#include "state_handler.h"

namespace fs = std::filesystem;

using ios = std::ios;

namespace state_handler {
//_________StateHandler_________
StateHandler::StateHandler(fs::path&& path) : path_(std::forward<fs::path>(path)) {
}

void StateHandler::SaveState(const app::ApplicationState& app_state) const {
    if(path_.empty()) {
        return;
    }

    std::ofstream out;
    fs::path tmp_file_name_ = "/tmp_" + path_.filename().string();
    fs::path tmp_file_path_ = path_.parent_path().string() + tmp_file_name_.string();
    
    out.open(tmp_file_path_, ios::out | ios::binary);

    if(!out) {
        throw std::runtime_error("Error open outfile");
    }

    OutputArchive oarchive{out};
    serialization::ApplicationStateRepr app_state_repr(app_state);
    oarchive << app_state_repr;
    out.close();

    fs::rename(tmp_file_path_, path_);
}

bool StateHandler::TryRestoreState(model::Game&& game, app::Application& app) {
    if(path_.empty()) {
        app.SetGame(std::move(game));
        return false;
    }

    std::ifstream input;
    input.open(path_, ios::in | ios::binary);

    //Возвращает true, т.к. означает что файла нет, или он не может быть открыт, или он пуст,
    //но запись игры должна осуществляться.
    if(!input || fs::is_empty(path_)) {
        app.SetGame(std::move(game));
        return true;
    }

    InputArchive iarchive{input};
    serialization::ApplicationStateRepr repr;
    iarchive >> repr;

    for(const auto& session_repr : repr.RestoreGameSessionsRepr()) {
        auto session = game.AddSession(session_repr.RestoreMapId());
        session->AddDogs(session_repr.RestoreDogs());
        session->AddLostObjects(session_repr.RestoreLoot());
    }
    app.SetGame(std::move(game));

    app.JoinGame(repr.RestorePlayersData());

    return true;
}

//_________SerrializingListener_________
SerializingListener::SerializingListener(std::chrono::milliseconds&& save_period, const app::Application& application, Handler handler) 
    : save_period_(std::forward<std::chrono::milliseconds>(save_period))
    , app_(application)
    , handler_(handler) {
}

void SerializingListener::OnTick(const std::chrono::milliseconds& delta) {
    using namespace std::chrono_literals;

    time_since_save_ += delta;
    if(time_since_save_ >= save_period_) {
        handler_(app_.GetApplicationState());
        time_since_save_ = 0ms;
    }
}
} // namespace state_handler
