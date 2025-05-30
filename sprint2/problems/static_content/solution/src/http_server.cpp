#include "http_server.h"
#include "logger.h"
#include "json_helper.h"

namespace http_server {

void ReportError(beast::error_code ec, std::string_view what) {
    logger::LogJsonAndMessage(json_helper::CreateNetworkErrorValue(ec.value(), ec.message(), std::string(what)), "error");
}

void ReportRequest(const tcp::endpoint& endpoint, std::string_view uri, std::string_view method) {
    logger::LogJsonAndMessage(json_helper::CreateRequestValue(endpoint, std::string(uri), std::string(method)), "request received");
}

void SessionBase::Run() {
    // Запуск метода Read с использованием executor объекта stream_.
    // Вся работа со stream_ будет выполняться через его executor
    net::dispatch(stream_.get_executor(),
                    beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    // Сброс запроса (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Чтение request_ из stream_ с использованием buffer_ для хранения данных
    http::async_read(stream_, buffer_, request_,
                        // По завершении операции вызывается метод OnRead
                        beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    if (ec == http::error::end_of_stream) {
        // Обычная ситуация — клиент закрыл соединение
        return Close();
    }
    
    if (ec) {
        return ReportError(ec, "read"sv);
    } else {
        ReportRequest(stream_.socket().local_endpoint(), request_.target(), request_.method_string());
    }
    

    HandleRequest(std::move(request_));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        return ReportError(ec, "write"sv);
    }

    if (close) {
        // По семантике ответа требуется закрыть соединение
        return Close();
    }

    // Чтение следующего запроса
    Read();
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

}  // namespace http_server
