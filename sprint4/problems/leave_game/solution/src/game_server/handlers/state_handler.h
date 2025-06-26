#pragma once

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <chrono>
#include <filesystem>

#include "../app/application.h"
#include "../app/detail/app_serializer.h"
#include "../model/dynamic_object_properties.h"

namespace state_handler {
using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

class StateHandler {
public:
    StateHandler() = default;
    explicit StateHandler(std::filesystem::path&& path);

    void SaveState(const app::ApplicationState& app_state) const;

    //Если вернул true значит поле, сожержащее путь, не пустое. 
    //В случае пустого файла, или если файл не был найден в Application записываются данные об игре из переданного аргумента.
    bool TryRestoreState(model::Game&& game, app::Application& app);   
private:
    std::filesystem::path path_ = "";
};

class SerializingListener : public app::ApplicationListener {
public:
    using Handler = std::function<void(app::ApplicationState)>;

    SerializingListener(std::chrono::milliseconds&& save_period, const app::Application& application, Handler handler);

    void OnTick(const std::chrono::milliseconds& delta) override;
private:
    std::chrono::milliseconds save_period_{0};
    std::chrono::milliseconds time_since_save_{0};
    
    const app::Application& app_;
    Handler handler_;
};
}//namespace state_handler