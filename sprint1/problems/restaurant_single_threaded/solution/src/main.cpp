#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory> // Для std::shared_ptr, std::enable_shared_from_this
#include <functional> // Для std::function
#include <mutex> // Хотя здесь не используется, часто нужен в многопоточных приложениях
#include <sstream>
#include <syncstream>
#include <string> // Для std::string, std::to_string
#include <stdexcept> // Для std::logic_error
#include <unordered_map>
#include <cassert> // Для assert

namespace net = boost::asio;
namespace sys = boost::system;
namespace ph = std::placeholders; // Не используется здесь, но может пригодиться для std::bind
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;

// --- Класс Hamburger (Предоставлен) ---
class Hamburger {
public:
    [[nodiscard]] bool IsCutletRoasted() const {
        return cutlet_roasted_;
    }
    void SetCutletRoasted() {
        if (IsCutletRoasted()) {
            throw std::logic_error("Cutlet has been roasted already"s);
        }
        cutlet_roasted_ = true;
    }

    [[nodiscard]] bool HasOnion() const {
        return has_onion_;
    }
    void AddOnion() {
        if (IsPacked()) {
            throw std::logic_error("Hamburger has been packed already"s);
        }
        AssureCutletRoasted();
        has_onion_ = true;
    }

    [[nodiscard]] bool IsPacked() const {
        return is_packed_;
    }
    void Pack() {
        AssureCutletRoasted();
        is_packed_ = true;
    }

private:
    void AssureCutletRoasted() const {
        if (!cutlet_roasted_) {
            // Исправлено сообщение об ошибке (было "Bread")
            throw std::logic_error("Cutlet has not been roasted yet"s);
        }
    }

    bool cutlet_roasted_ = false;
    bool has_onion_ = false;
    bool is_packed_ = false;
};

std::ostream& operator<<(std::ostream& os, const Hamburger& h) {
    return os << "Hamburger: "sv << (h.IsCutletRoasted() ? "roasted cutlet"sv : " raw cutlet"sv)
              << (h.HasOnion() ? ", onion"sv : ""sv)
              << (h.IsPacked() ? ", packed"sv : ", not packed"sv);
}

// --- Класс Logger (Предоставлен) ---
class Logger {
public:
    explicit Logger(std::string id)
        : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

// --- Псевдоним типа OrderHandler (Предоставлен) ---
using OrderHandler = std::function<void(sys::error_code ec, int id, Hamburger* hamburger)>;

// --- Класс Order (Реализация асинхронного заказа) ---
class Order : public std::enable_shared_from_this<Order> {
public:
    Order(net::io_context& io, int id, bool with_onion, OrderHandler handler)
        : io_{io}
        , id_{id}
        , with_onion_{with_onion}
        , handler_{std::move(handler)}
        , logger_{std::to_string(id)}
        , roast_timer_{io}      // Таймер для жарки котлеты
        , marinade_timer_{io}   // Таймер для маринования лука
        {
    }

    // Запускает асинхронное выполнение заказа
    void Execute() {
        logger_.LogMessage("Order has been started."sv);
        RoastCutlet();
        if (with_onion_) {
            MarinadeOnion();
        }
    }
private:
    // Асинхронная жарка котлеты (1 секунда)
    void RoastCutlet() {
        logger_.LogMessage("Start roasting cutlet"sv);
        roast_timer_.expires_after(1s);
        // Захватываем self через shared_from_this(), чтобы Order жил до выполнения колбэка
        roast_timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnRoasted(ec);
        });
    }

    // Колбэк завершения жарки котлеты
    void OnRoasted(sys::error_code ec) {
        if (ec) {
             if (ec != net::error::operation_aborted) { // Игнорируем отмену операции
                 logger_.LogMessage("Roast error : "s + ec.message());
             }
        } else {
            logger_.LogMessage("Cutlet has been roasted."sv);
            try {
                 hamburger_.SetCutletRoasted();
            } catch (const std::logic_error& e) {
                 logger_.LogMessage("Logic error during setting roasted cutlet: "s + e.what());
                 ec = net::error::invalid_argument; // Используем стандартный код ошибки
            }
        }
        CheckReadiness(ec); // Проверяем готовность после завершения операции
    }

    // Асинхронное маринование лука (2 секунды)
    void MarinadeOnion() {
        logger_.LogMessage("Start marinading onion"sv);
        marinade_timer_.expires_after(2s);
        // Захватываем self через shared_from_this()
        marinade_timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnOnionMarinaded(ec);
        });
    }

    // Колбэк завершения маринования лука
    void OnOnionMarinaded(sys::error_code ec) {
        if (ec) {
             if (ec != net::error::operation_aborted) {
                logger_.LogMessage("Marinade onion error: "s + ec.message());
            }
        } else {
            logger_.LogMessage("Onion has been marinaded."sv);
            onion_marinaded_ = true;
        }
        CheckReadiness(ec); // Проверяем готовность после завершения операции
    }

    // Проверяет готовность ингредиентов и переходит к следующему шагу
    void CheckReadiness(sys::error_code ec) {
        if (delivered_) { // Если уже доставили (или была ошибка), ничего не делаем
            return;
        }
        if (ec) { // Если пришла ошибка от одной из операций
            return Deliver(ec); // Сообщаем об ошибке
        }

        // Пытаемся добавить лук, если условия выполнены
        if (CanAddOnion()) {
             try {
                 logger_.LogMessage("Add onion"sv);
                 hamburger_.AddOnion();
             } catch (const std::logic_error& e) {
                logger_.LogMessage("Logic error during adding onion: "s + e.what());
                return Deliver(net::error::invalid_argument);
            }
        }

        // Проверяем, можно ли упаковывать
        if (IsReadyToPack()) {
            Pack(); // Начинаем синхронную упаковку
        }
    }

    // Имитирует синхронную упаковку (0.5 секунды)
    void Pack() {
        if (hamburger_.IsPacked()) { // Предотвращаем повторную упаковку
            return;
        }

        logger_.LogMessage("Packing"sv);

        // Имитация работы процессора в течение 0.5 с
        auto start = steady_clock::now();
        while (steady_clock::now() - start < 500ms) {
            // Активное ожидание
        }

        try {
            hamburger_.Pack();
            logger_.LogMessage("Packed"sv);
            Deliver({}); // Доставляем успешный результат (пустой код ошибки)
        } catch (const std::logic_error& e) {
             logger_.LogMessage("Logic error during packing: "s + e.what());
             Deliver(net::error::invalid_argument);
        }

    }

    // Вызывает обработчик для доставки результата (или ошибки)
    void Deliver(sys::error_code ec) {
        if (delivered_) { // Гарантируем однократную доставку
            return;
        }
        delivered_ = true;
        // Вызываем внешний обработчик, переданный в конструктор Order
        handler_(ec, id_, ec ? nullptr : &hamburger_);
        // После этого Order может быть уничтожен, если на него не осталось shared_ptr
    }

    // Проверка, можно ли добавить лук
    [[nodiscard]] bool CanAddOnion() const {
        return with_onion_ && hamburger_.IsCutletRoasted() && onion_marinaded_ && !hamburger_.HasOnion();
    }

    // Проверка, готов ли гамбургер к упаковке
    [[nodiscard]] bool IsReadyToPack() const {
        return hamburger_.IsCutletRoasted() && (!with_onion_ || hamburger_.HasOnion()) && !hamburger_.IsPacked();
    }

    net::io_context& io_;
    int id_;
    bool with_onion_;
    OrderHandler handler_;
    Logger logger_;
    Hamburger hamburger_;
    Timer roast_timer_;
    Timer marinade_timer_;
    bool onion_marinaded_ = false; // Замаринован ли лук?
    bool delivered_ = false;       // Доставлен ли заказ?
};

// --- Класс Restaurant (Реализация) ---
class Restaurant {
public:
    explicit Restaurant(net::io_context& io)
        : io_(io) {
    }

    // Создает Order и запускает его выполнение
    int MakeHamburger(bool with_onion, OrderHandler handler) {
        const int order_id = ++next_order_id_;
        // Создаем Order через make_shared, чтобы он жил достаточно долго для асинхронных операций
        std::make_shared<Order>(io_, order_id, with_onion, std::move(handler))->Execute();
        return order_id;
    }

private:
    net::io_context& io_;
    int next_order_id_ = 0;
};

// --- Функция main (Предоставлена для тестов) ---
int main() {
    net::io_context io;

    Restaurant restaurant{io};

    Logger logger{"main"s};

    struct OrderResult {
        sys::error_code ec;
        Hamburger hamburger;
    };

    std::unordered_map<int, OrderResult> orders;
    // Лямбда-функция для обработки результата заказа
    auto handle_result = [&orders, &logger](sys::error_code ec, int id, Hamburger* h) {
        orders.emplace(id, OrderResult{ec, ec ? Hamburger{} : *h});
         if (ec) {
            logger.LogMessage("Order "s + std::to_string(id) + " failed: " + ec.message());
        } else {
            std::ostringstream oss;
            oss << "Order " << id << " is ready. " << *h;
            logger.LogMessage(oss.str());
        }
    };

    logger.LogMessage("Ordering burgers..."sv);
    const int id1 = restaurant.MakeHamburger(false, handle_result); // Без лука
    const int id2 = restaurant.MakeHamburger(true, handle_result);  // С луком

    // До io.run() заказы не выполняются
    assert(orders.empty());
    logger.LogMessage("Running io_context..."sv);
    io.run(); // Запускаем обработку асинхронных операций
    logger.LogMessage("io_context finished."sv);


    // После io.run() все заказы должны быть выполнены
    assert(orders.size() == 2u);
    {
        // Проверяем заказ без лука
        logger.LogMessage("Checking order "s + std::to_string(id1));
        const auto& o = orders.at(id1);
        assert(!o.ec);
        assert(o.hamburger.IsCutletRoasted());
        assert(o.hamburger.IsPacked());
        assert(!o.hamburger.HasOnion());
        logger.LogMessage("Order "s + std::to_string(id1) + " verified.");
    }
    {
        // Проверяем заказ с луком
         logger.LogMessage("Checking order "s + std::to_string(id2));
        const auto& o = orders.at(id2);
        assert(!o.ec);
        assert(o.hamburger.IsCutletRoasted());
        assert(o.hamburger.IsPacked());
        assert(o.hamburger.HasOnion());
        logger.LogMessage("Order "s + std::to_string(id2) + " verified.");
    }

    logger.LogMessage("All tests passed.");

    // Пример заказа большего количества гамбургеров, как в теории
    orders.clear();
    logger.LogMessage("\nOrdering 4 burgers (2 with onion, 2 without)..."sv);
     for (int i = 1; i <= 4; ++i) {
        // Заказы 1, 3 - с луком; 2, 4 - без лука
        restaurant.MakeHamburger(i % 2 != 0, handle_result);
    }
    logger.LogMessage("Running io_context again..."sv);
    // Если io_context остановился, нужно его перезапустить перед новым io.run()
    io.restart();
    io.run();
    logger.LogMessage("io_context finished again.");
    assert(orders.size() == 4u);
    logger.LogMessage("Successfully processed 4 more burgers.");


    return 0; // Явно возвращаем 0 из main
}
