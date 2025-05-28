#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <iomanip>
#include <sstream>
#include "json.hpp"
#include <mutex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace fs = boost::filesystem;
using tcp = net::ip::tcp;
using nlohmann::json;

std::string get_iso_timestamp() {
    using namespace std::chrono;
    std::ostringstream ss;
    try {
        auto now = system_clock::now();
        time_t itt = system_clock::to_time_t(now);
        std::tm ptm_buf;
        // Use gmtime_r or gmtime_s for thread-safety if available and needed, 
        // but std::gmtime is often fine for single-threaded access or if locale isn't an issue.
        // For this context, checking for nullptr is the main concern.
        std::tm* ptm = std::gmtime(&itt); // In C++17, can use gmtime_r with a buffer if available
                                      // For older standards or some compilers, gmtime might use a static buffer.
                                      // However, the immediate issue is nullptr return.
#ifdef _MSC_VER
        // Visual Studio provides gmtime_s
        if (gmtime_s(&ptm_buf, &itt) == 0) {
            ptm = &ptm_buf;
        } else {
            ptm = nullptr;
        }
#else
        // For GCC/Clang, gmtime_r is common, or check if std::gmtime is safe enough
        // For simplicity here, just use std::gmtime and check for null.
        // If thread safety for a static buffer used by gmtime becomes an issue later,
        // this would need a mutex or thread-safe version like gmtime_r.
        // ptm = gmtime_r(&itt, &ptm_buf); // if available & using C-style
#endif

        if (ptm) {
            ss << std::put_time(ptm, "%FT%T");
            auto ms_duration = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;
            ss << '.' << std::setfill('0') << std::setw(6) << ms_duration.count();
        } else {
            ss << "1970-01-01T00:00:00.000000Z_GMTIME_FAILED"; 
        }
    } catch (const std::exception& e) {
        // Clear stream state in case of partial writes before exception
        ss.str(""); 
        ss.clear(); 
        ss << "1970-01-01T00:00:00.000000Z_TIMESTAMP_EXCEPTION:" << e.what();
    } catch (...) {
        ss.str("");
        ss.clear();
        ss << "1970-01-01T00:00:00.000000Z_UNKNOWN_TIMESTAMP_ERROR";
    }
    return ss.str();
}

void log_json(const std::string& message, const json& data) {
    json obj;
    obj["timestamp"] = get_iso_timestamp();
    obj["message"] = message;
    obj["data"] = data;
    std::cout << obj.dump() << std::endl << std::flush;
}

std::string get_content_type(const std::string& path) {
    if (path.length() >= 5 && path.substr(path.length() - 5) == ".html") return "text/html";
    if (path.length() >= 4 && path.substr(path.length() - 4) == ".svg") return "image/svg+xml";
    if (path.length() >= 3 && path.substr(path.length() - 3) == ".js") return "application/javascript";
    if (path.length() >= 5 && path.substr(path.length() - 5) == ".json") return "application/json";
    return "text/plain";
}

std::vector<json> g_maps;
std::map<std::string, json> g_map_by_id;
std::once_flag g_maps_loaded;

void load_maps() {
    std::ifstream config_file("data/config.json");
    if (!config_file) return;
    json config;
    config_file >> config;
    for (const auto& m : config["maps"]) {
        g_maps.push_back({{"id", m["id"]}, {"name", m["name"]}});
        g_map_by_id[m["id"]] = m;
    }
}

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void run() { read_request(); }

private:
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    std::string client_ip_;

    void read_request() {
        try {
        client_ip_ = socket_.remote_endpoint().address().to_string();
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_,
            [self](beast::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec == beast::http::error::end_of_stream) {
                            return self->do_close();
                        }
                        json error_data = {
                            {"code", ec.value()},
                            {"text", ec.message()},
                            {"where", "read"}
                        };
                        log_json("error", error_data);
                        return self->do_close();
                    }
                    self->process_request();
            });
        } catch (const std::exception& e) {
            json error_data = {
                {"code", 1},
                {"text", e.what()},
                {"where", "read"}
            };
            log_json("error", error_data);
            do_close();
        }
    }

    void do_close() {
        beast::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        if (ec) {
            json error_data = {
                {"code", ec.value()},
                {"text", ec.message()},
                {"where", "shutdown_in_do_close"}
            };
            log_json("error", error_data);
        }
    }

    void process_request() {
        res_.result(http::status::internal_server_error);
        res_.clear();
        res_.version(req_.version());
        res_.keep_alive(req_.keep_alive());
        
        std::string current_request_uri = req_.target().to_string();
        std::string current_request_method = req_.method_string().to_string();
        auto start_time_point = std::chrono::steady_clock::now();

        try {
        std::call_once(g_maps_loaded, load_maps);
        // Log request
        json req_data = {
            {"ip", client_ip_},
                {"URI", current_request_uri},
                {"method", current_request_method}
        };
        log_json("request received", req_data);

        std::string target = req_.target().to_string();
        bool handled = false;
        if (target == "/info") {
                res_.result(http::status::ok);
                res_.set(http::field::content_type, "application/json");
            json info = {{"name", "Static Content Server"}, {"version", "1.0"}, {"code", 200}};
                res_.body() = info.dump();
            handled = true;
        } else if (target == "/api/v1/maps") {
                res_.result(http::status::ok);
                res_.set(http::field::content_type, "application/json");
                res_.body() = json(g_maps).dump();
            handled = true;
        } else if (target.find("/api/v1/maps/") == 0) {
            std::string map_id = target.substr(std::string("/api/v1/maps/").size());
            if (g_map_by_id.count(map_id)) {
                    res_.result(http::status::ok);
                    res_.set(http::field::content_type, "application/json");
                    res_.body() = g_map_by_id[map_id].dump();
            } else {
                    res_.result(http::status::not_found);
                    res_.set(http::field::content_type, "application/json");
                json err = {{"code", "mapNotFound"}, {"message", "Map not found"}};
                    res_.body() = err.dump();
            }
            handled = true;
        } else if (target.find("/api/") == 0) {
                res_.result(http::status::bad_request);
                res_.set(http::field::content_type, "application/json");
            json err = {{"code", "badRequest"}, {"message", "Bad request"}};
                res_.body() = err.dump();
            handled = true;
        }
        if (!handled) {
            if (target == "/") target = "/index.html";
            fs::path file_path = fs::current_path() / "static" / target.substr(1);
            if (!fs::exists(file_path) || fs::is_directory(file_path)) {
                    res_.result(http::status::not_found);
                    res_.set(http::field::content_type, "text/plain");
                    res_.body() = "File not found";
            } else {
                std::ifstream file(file_path.string(), std::ios::in | std::ios::binary);
                if (!file) {
                        res_.result(http::status::internal_server_error);
                        res_.set(http::field::content_type, "text/plain");
                        res_.body() = "Internal server error";
                } else {
                    std::ostringstream ss;
                        ss << file.rdbuf(); // This operation can throw std::ios_base::failure
                        res_.result(http::status::ok);
                        res_.set(http::field::content_type, get_content_type(file_path.string()));
                        res_.body() = ss.str();
                }
            }
        }
        } catch (const std::exception& e) {
            // Log the internal error
            json error_details = {
                {"ip", client_ip_},
                {"URI", current_request_uri},
                {"method", current_request_method},
                {"exception", e.what()},
                {"where", "process_request"}
            };
            log_json("error", error_details);

            // Prepare a 500 Internal Server Error response
            res_.result(http::status::internal_server_error);
            res_.set(http::field::content_type, "application/json");
            json error_response_payload = {
                {"code", "internalError"},
                {"message", "An internal server error occurred."}
            };
            res_.body() = error_response_payload.dump();
        } catch (...) { // Catch any other non-std::exception
            json error_details = {
                {"ip", client_ip_},
                {"URI", current_request_uri},
                {"method", current_request_method},
                {"exception", "Unknown exception type"},
                {"where", "process_request_unknown"}
            };
            log_json("error", error_details);

            res_.result(http::status::internal_server_error);
            res_.set(http::field::content_type, "application/json");
            json error_response_payload = {
                {"code", "internalError"},
                {"message", "An unknown internal server error occurred."}
            };
            res_.body() = error_response_payload.dump();
        }

        res_.prepare_payload();

        auto end_time_point = std::chrono::steady_clock::now();
        auto response_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time_point - start_time_point).count();

        // Log response sent
        json final_res_data = {
            {"ip", client_ip_},
            {"response_time", response_time_ms},
            {"code", res_.result_int()},
            {"content_type", res_[http::field::content_type].to_string()}
        };
        log_json("response sent", final_res_data);

        auto self = shared_from_this();
        http::async_write(socket_, res_,
            [self](beast::error_code ec, std::size_t) {
                if (ec) {
                    json error_data = {
                        {"code", ec.value()},
                        {"text", ec.message()},
                        {"where", "write"}
                    };
                    log_json("error", error_data);
                    return self->do_close();
                }

                if (!self->req_.keep_alive()) {
                    return self->do_close();
                }

                self->buffer_.consume(self->buffer_.size());
                self->read_request();
            });
    }
};

void do_accept(tcp::acceptor& acceptor) {
    acceptor.async_accept(
        [&acceptor](beast::error_code ec, tcp::socket socket) {
            if (ec) {
                json error_data = {
                    {"code", ec.value()},
                    {"text", ec.message()},
                    {"where", "accept"}
                };
                log_json("error", error_data);
            } else {
                std::make_shared<HttpSession>(std::move(socket))->run();
            }
            if (acceptor.is_open()) {
            do_accept(acceptor);
            }
        });
}

int main() {
    try {
        // Initialize Boost logger with auto_flush enabled
        boost::log::add_common_attributes();
        boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
        boost::log::add_console_log(std::cout, boost::log::keywords::auto_flush = true);

        // First, setup basic logging without any complex operations
        json start_data = {{"port", 8080}, {"address", "0.0.0.0"}};
        log_json("Server has started", start_data);  // This matches the test's expected format with capture group

        // Setup network components
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc};
        
        // Configure acceptor with detailed error handling
        boost::system::error_code ec;
        tcp::endpoint endpoint(net::ip::make_address("0.0.0.0"), 8080);
        
        acceptor.open(endpoint.protocol(), ec);
        if (ec) {
            log_json("error", {{"code", ec.value()}, {"text", ec.message()}, {"where", "acceptor.open"}});
            return 1;
        }
        
        acceptor.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            log_json("error", {{"code", ec.value()}, {"text", ec.message()}, {"where", "acceptor.set_option"}});
            return 1;
        }
        
        acceptor.bind(endpoint, ec);
        if (ec) {
            log_json("error", {{"code", ec.value()}, {"text", ec.message()}, {"where", "acceptor.bind"}});
            return 1;
        }
        
        acceptor.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            log_json("error", {{"code", ec.value()}, {"text", ec.message()}, {"where", "acceptor.listen"}});
            return 1;
        }

        // Log that server is ready to accept connections
        log_json("server listening and ready", start_data);
        std::cout << std::flush;

        // Start accepting connections
        do_accept(acceptor);

        // Run the event loop
        ioc.run();
        log_json("server event loop finished", {});

        log_json("server exited cleanly", {{"code", 0}});
        return 0;
    } catch (const std::exception& e) {
        log_json("error", {{"code", 1}, {"text", e.what()}, {"where", "main_exception"}});
        log_json("server exited with exception", {{"code", 1}, {"exception", e.what()}});
        return 1;
    } catch (...) {
        log_json("error", {{"text", "Unknown exception in main"}, {"where", "main_unknown_exception"}});
        log_json("server exited with unknown exception", {{"code", 2}, {"exception", "Unknown exception type"}});
        return 2;
    }
} 