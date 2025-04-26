#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/post.hpp> // Для net::post
#include <boost/asio/dispatch.hpp> // Для net::dispatch
#include <memory>
#include <mutex> // Для std::mutex и std::lock_guard (хотя не используются напрямую)
#include <atomic> // Для std::atomic
#include <stdexcept> // Для runtime_error

#include "hotdog.h"
#include "result.h"
#include "ingredients.h" // Подключаем ingredients.h
#include "gascooker.h"   // Подключаем gascooker.h
#include "clock.h"       // Подключаем clock.h

namespace net = boost::asio;
namespace sys = boost::system; // Добавим для error_code

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Вспомогательный класс для управления приготовлением одного хот-дога
class HotDogOrder : public std::enable_shared_from_this<HotDogOrder> {
public:
    HotDogOrder(net::io_context& io, Store& store, GasCooker& cooker, HotDogHandler handler, int hot_dog_id)
        : strand_{net::make_strand(io)}, // Strand для синхронизации этого заказа
          store_{store},
          cooker_{cooker},
          handler_{std::move(handler)}, // Перемещаем обработчик
          bread_timer_{strand_}, // Таймеры ассоциируем со strand'ом заказа
          sausage_timer_{strand_},
          hot_dog_id_{hot_dog_id}
          {}

    // Запускает процесс приготовления
    void Start() {
        // Выполняем начальные действия в strand'е заказа
        // Используем dispatch для немедленного выполнения, если мы уже в strand'е,
        // или post, если вызывается из другого потока.
        net::dispatch(strand_, [self = shared_from_this()] {
            try {
                self->bread_ = self->store_.GetBread(); // Получаем ингредиенты
                self->sausage_ = self->store_.GetSausage();

                // Начинаем готовить оба ингредиента параллельно
                self->StartBakingBread();
                self->StartFryingSausage();
            } catch (...) {
                 // Ошибка при получении ингредиентов (маловероятно, но возможно)
                 self->error_ = std::current_exception();
                 // Устанавливаем флаги готовности, чтобы CheckReadiness сработал
                 self->bread_ready_ = true;
                 self->sausage_ready_ = true;
                 self->CheckReadiness();
            }
        });
    }

private:
    // Начинает выпечку хлеба и устанавливает таймер
    void StartBakingBread() {
        // Захватываем self для продления жизни заказа
        bread_->StartBake(cooker_, [self = shared_from_this()] {
             // Этот колбэк вызывается, когда горелка для хлеба занята.
             // Выполняется в произвольном потоке io_context.
             // Используем post для выполнения установки таймера в нашем strand_.
             net::post(self->strand_, [self] {
                if(self->delivered_) return; // Проверка на случай, если ошибка пришла раньше
                // Запускаем таймер выпечки на 1 секунду
                self->bread_timer_.expires_after(Milliseconds{1000});
                self->bread_timer_.async_wait([self](const sys::error_code& ec){
                    self->OnBreadBaked(ec);
                });
             });
        });
    }

    // Начинает жарку сосиски и устанавливает таймер
    void StartFryingSausage() {
         // Захватываем self для продления жизни заказа
        sausage_->StartFry(cooker_, [self = shared_from_this()] {
            // Этот колбэк вызывается, когда горелка для сосиски занята.
            // Выполняется в произвольном потоке io_context.
            // Используем post для выполнения установки таймера в нашем strand_.
            net::post(self->strand_, [self] {
                 if(self->delivered_) return; // Проверка на случай, если ошибка пришла раньше
                // Запускаем таймер жарки на 1.5 секунды
                self->sausage_timer_.expires_after(Milliseconds{1500});
                self->sausage_timer_.async_wait([self](const sys::error_code& ec){
                    self->OnSausageFried(ec);
                });
            });
        });
    }

    // Колбэк таймера выпечки хлеба (выполняется в strand_)
    void OnBreadBaked(sys::error_code ec) {
        if (delivered_) return; // Если уже доставили, выходим

        if (ec == net::error::operation_aborted) {
            // Таймер был отменен, возможно, из-за ошибки в другом месте.
            // Не устанавливаем ошибку здесь, она уже должна быть установлена.
        } else if (ec) {
            // Другая ошибка таймера
            if (!error_) error_ = std::make_exception_ptr(std::runtime_error("Bread timer error: " + ec.message()));
        } else {
            // Таймер сработал успешно, останавливаем выпечку
            try {
                 bread_->StopBaking(); // Останавливаем выпечку (освобождаем горелку)
            } catch (...) {
                // Ошибка при остановке выпечки
                if (!error_) error_ = std::current_exception();
            }
        }
        bread_ready_ = true;
        // Отменяем таймер сосиски, если он еще работает и у нас уже есть ошибка
        if (error_ && sausage_timer_.cancel() > 0) {
            // Если таймер сосиски был успешно отменен, колбек OnSausageFried
            // будет вызван с ошибкой operation_aborted.
            // Нам нужно вручную пометить сосиску как "готовую" (в смысле завершения операции)
            // чтобы CheckReadiness сработал и доставил ошибку.
            sausage_ready_ = true;
        }
        CheckReadiness(); // Проверяем, готовы ли оба ингредиента
    }

     // Колбэк таймера жарки сосиски (выполняется в strand_)
    void OnSausageFried(sys::error_code ec) {
        if (delivered_) return;

        if (ec == net::error::operation_aborted) {
           // Таймер отменен
        } else if (ec) {
             // Другая ошибка таймера
             if (!error_) error_ = std::make_exception_ptr(std::runtime_error("Sausage timer error: " + ec.message()));
        } else {
             // Таймер сработал успешно, останавливаем жарку
             try {
                sausage_->StopFry(); // Останавливаем жарку
             } catch (...) {
                 if (!error_) error_ = std::current_exception();
             }
        }
        sausage_ready_ = true;
        // Отменяем таймер хлеба, если он еще работает и у нас уже есть ошибка
        if (error_ && bread_timer_.cancel() > 0) {
            bread_ready_ = true; // Помечаем как "готовый" для CheckReadiness
        }
        CheckReadiness();
    }

    // Проверка готовности и сборка/доставка хот-дога (выполняется в strand_)
    void CheckReadiness() {
        // Дополнительная синхронизация не нужна, т.к. мы в strand_

        if (delivered_) return; // Уже доставлено

        // Если обе операции завершены (успешно или с ошибкой)
        if (bread_ready_ && sausage_ready_) {
             if (error_) {
                 // Если была ошибка на каком-то этапе, доставляем ее
                 Deliver(Result<HotDog>{error_});
             } else {
                 // Ошибок не было, пытаемся собрать хот-дог
                 try {
                     // Конструктор HotDog проверяет время приготовления
                     HotDog hot_dog(hot_dog_id_, sausage_, bread_);
                     Deliver(Result<HotDog>{std::move(hot_dog)});
                 } catch (...) {
                     // Ошибка при сборке (неверное время приготовления или другая)
                     Deliver(Result<HotDog>::FromCurrentException());
                 }
             }
        }
    }

    // Доставка результата
    void Deliver(Result<HotDog> result) {
        if (!delivered_) {
            delivered_ = true;
            // Отменяем оставшиеся таймеры на всякий случай
            bread_timer_.cancel();
            sausage_timer_.cancel();

            // !!! ИСПРАВЛЕНИЕ !!!
            // Вызываем внешний обработчик через post НА СТРЕНД ЗАКАЗА.
            // Это гарантирует, что handler выполнится после завершения Deliver
            // и в той же последовательности, что и другие операции этого заказа.
            // Захватываем handler по значению.
            net::post(strand_, [handler = handler_, result = std::move(result)]() mutable {
                 handler(std::move(result));
            });
            // После вызова handler объект HotDogOrder может быть уничтожен.
        }
    }


    using Strand = net::strand<net::io_context::executor_type>;

    Strand strand_; // Strand для синхронизации состояния этого заказа
    Store& store_;
    GasCooker& cooker_;
    HotDogHandler handler_; // Храним обработчик по значению (или shared_ptr, если нужно)
    int hot_dog_id_;

    std::shared_ptr<Bread> bread_;
    std::shared_ptr<Sausage> sausage_;
    net::steady_timer bread_timer_;
    net::steady_timer sausage_timer_;

    bool bread_ready_ = false;
    bool sausage_ready_ = false;
    bool delivered_ = false;
    std::exception_ptr error_ = nullptr; // Хранение первой возникшей ошибки
};


// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io},
          // Инициализируем strand для доступа к store_ и next_hotdog_id_
          order_strand_{net::make_strand(io_)}
           {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        // Используем dispatch для выполнения кода в order_strand_
        // Это гарантирует потокобезопасный доступ к store_ и next_hotdog_id_
        // Захватываем handler по значению (копируем или перемещаем)
        net::dispatch(order_strand_, [this, handler = std::move(handler)]() mutable {
            const int order_id = next_hotdog_id_++;
            // Создаем и запускаем новый заказ
            // Передаем handler дальше по значению (перемещаем)
            std::make_shared<HotDogOrder>(io_, store_, *gas_cooker_, std::move(handler), order_id)->Start();
        });
    }

private:
    net::io_context& io_;
    // Strand для синхронизации создания заказов
    net::strand<net::io_context::executor_type> order_strand_;
    std::atomic<int> next_hotdog_id_{0}; // Используем atomic для генерации ID
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
