#pragma once
#include <functional>
#include <optional>
#include <stdexcept> // Добавлено для std::logic_error
#include <memory>    // Добавлено для enable_shared_from_this

#include "clock.h"
#include "gascooker.h"

/*
Класс "Сосиска".
Позволяет себя обжаривать на газовой плите
*/
class Sausage : public std::enable_shared_from_this<Sausage> {
public:
    using Handler = std::function<void()>;

    explicit Sausage(int id)
        : id_{id} {
    }

    int GetId() const {
        return id_;
    }

    // Асинхронно начинает приготовление. Вызывает handler, как только началось приготовление
    void StartFry(GasCooker& cooker, Handler handler) {
        // Метод StartFry можно вызвать только один раз
        if (frying_start_time_) {
            throw std::logic_error("Frying already started");
        }

        // Используем как флаг, фактическое время запишется при захвате горелки
        frying_start_time_ = Clock::now();

        // Готовимся занять газовую плиту
        gas_cooker_lock_ = GasCookerLock{cooker.shared_from_this()};

        // Занимаем горелку для начала обжаривания.
        // Чтобы продлить жизнь текущего объекта, захватываем shared_ptr в лямбде
        cooker.UseBurner([self = shared_from_this(), handler = std::move(handler)] {
            // Запоминаем время фактического начала обжаривания
            self->frying_start_time_ = Clock::now();
            handler();
        });
    }

    // Завершает приготовление и освобождает горелку
    void StopFry() {
        if (!frying_start_time_) {
            throw std::logic_error("Frying has not started");
        }
        if (frying_end_time_) {
            throw std::logic_error("Frying has already stopped");
        }
        frying_end_time_ = Clock::now();
        // Освобождаем горелку
        gas_cooker_lock_.Unlock();
    }

    bool IsCooked() const noexcept {
        return frying_start_time_.has_value() && frying_end_time_.has_value();
    }

    Clock::duration GetCookDuration() const {
        if (!IsCooked()) {
            throw std::logic_error("Sausage has not been cooked");
        }
        return *frying_end_time_ - *frying_start_time_;
    }

private:
    int id_;
    GasCookerLock gas_cooker_lock_;
    std::optional<Clock::time_point> frying_start_time_;
    std::optional<Clock::time_point> frying_end_time_;
};

// Класс "Хлеб". Ведёт себя аналогично классу "Сосиска"
class Bread : public std::enable_shared_from_this<Bread> {
public:
    using Handler = std::function<void()>;

    explicit Bread(int id)
        : id_{id} {
    }

    int GetId() const {
        return id_;
    }

    // Начинает приготовление хлеба на газовой плите. Как только горелка будет занята, вызовет
    // handler
    void StartBake(GasCooker& cooker, Handler handler) {
        if (baking_start_time_) {
            throw std::logic_error("Baking already started");
        }
        // Используем как флаг, фактическое время запишется при захвате горелки
        baking_start_time_ = Clock::now();

        gas_cooker_lock_ = GasCookerLock{cooker.shared_from_this()};

        // Занимаем горелку асинхронно
        cooker.UseBurner([self = shared_from_this(), handler = std::move(handler)] {
            // Горелка занята, запоминаем точное время начала выпекания
            self->baking_start_time_ = Clock::now();
            // Уведомляем внешний код, что выпечка началась
            handler();
        });
    }

    // Останавливает приготовление хлеба и освобождает горелку.
    void StopBaking() {
         if (!baking_start_time_) {
            throw std::logic_error("Baking has not started");
        }
        if (baking_end_time_) {
            throw std::logic_error("Baking has already stopped");
        }
        baking_end_time_ = Clock::now();
        // Освобождаем горелку через RAII объект
        gas_cooker_lock_.Unlock();
    }

    // Информирует, испечён ли хлеб
    bool IsCooked() const noexcept {
        return baking_start_time_.has_value() && baking_end_time_.has_value();
    }

    // Возвращает продолжительность выпекания хлеба. Бросает исключение, если хлеб не был испечён
    Clock::duration GetBakingDuration() const {
        if (!IsCooked()) {
             throw std::logic_error("Bread has not been baked");
        }
        // Dereference optional values
        return *baking_end_time_ - *baking_start_time_;
    }

private:
    int id_;
    GasCookerLock gas_cooker_lock_; // RAII для горелки
    std::optional<Clock::time_point> baking_start_time_; // Время начала выпечки
    std::optional<Clock::time_point> baking_end_time_;   // Время окончания выпечки
};

// Склад ингредиентов (возвращает ингредиенты с уникальным id)
class Store {
public:
    std::shared_ptr<Bread> GetBread() {
        return std::make_shared<Bread>(++next_id_);
    }

    std::shared_ptr<Sausage> GetSausage() {
        return std::make_shared<Sausage>(++next_id_);
    }

private:
    // Используем atomic для потокобезопасного инкремента ID, хотя доступ к store будет
    // синхронизирован через strand в Cafeteria, но так надежнее.
    std::atomic<int> next_id_{0};
};
