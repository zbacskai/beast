#include <Session.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

namespace {

    // HTTP responses
    template<class Body, class Allocator, class Send>
    void handle_request(
        http::request<Body, http::basic_fields<Allocator>>&& req,
        Send&& send) 
    {
        // Return response to bad request
        auto const bad_request = [&req](beast::string_view reason)
        {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(reason);
            res.prepare_payload();
            return res;
        };

        // Return response not found
        auto const not_found = [&req](beast::string_view target)
        {
            http::response<http::string_body> res{http::status::not_found,req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            std::string body_msg = "Resource '" + std::string(target) + "' not found.";
            res.body() = body_msg;
            res.content_length(body_msg.size());
            res.prepare_payload();
            return res;                      
        };

        // Return response server error
        auto const server_error = [&req](beast::string_view server_error_msg)
        {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "Server Error: " + std::string(server_error_msg);
            res.prepare_payload();
            return res;                      
        };

        // Check the method type (GET/HEAD) supported now
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head)
            return send(bad_request("Not supported HTTP-method"));

        // Check request path
        if (req.target().empty() || req.target() != "/" || req.target().find("..") != beast::string_view::npos)
            return send(not_found("Check URL! Target not found!"));

        // Check cookies
        auto it = req.find(http::field::cookie);
        if (it != req.end())
        {
            for(auto param : http::param_list(std::string(";")+req[http::field::cookie].data()))
            {
                std::cout << "Cookie '" << param.first << "' has value " << param.second << "'\n";
            }
        }

        std::string ok_response = "{ \"status\" : \"200\" }";

        // Respond to HEAD request
        if(req.method() == http::verb::head)
        {
            auto const head_response = [&req, &ok_response]() {
                http::response<http::string_body> res{http::status::ok, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "application/json");
                res.content_length(ok_response.size());
                res.keep_alive(req.keep_alive());
                res.prepare_payload();
                return res;
            };
            return send(head_response());
        }
    

        auto const ok_resp = [&req, &ok_response]()
        {
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "application/json");
            res.content_length(ok_response.size());
            res.keep_alive(req.keep_alive());
            res.body() = ok_response;
            res.set(http::field::set_cookie, "z=123");
            res.insert(http::field::set_cookie, "x=456");
            res.prepare_payload();
            return res;
        };

        return send(ok_resp());
    }

    // Send failure
    void fail(beast::error_code ec, char const* error_msg) {
        std::cerr << error_msg << ": " << ec.message() << std::endl;
    }

    struct send_lambda {
        beast::tcp_stream& stream_;
        bool& close_;
        beast::error_code& ec_;
        net::yield_context yield_;

        send_lambda(
            beast::tcp_stream& stream,
            bool& close,
            beast::error_code& ec,
            net::yield_context yield) : stream_(stream), close_(close), ec_(ec), yield_(yield) {}

        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // Check if connetion needs to be terminated
            close_ = msg.need_eof();

            // We need serializer here.
            http::serializer<isRequest, Body, Fields> sr{msg};
            http::async_write(stream_, sr, yield_[ec_]);
        }
    };
} // end of anonymus namespace

Session::Session(tcp::socket&& socket) : _stream(std::move(socket)) {}

void Session::operator()(net::yield_context yield)
{
    std::cout << "--- START ---" << std::endl;
    bool close_conn = false;
    beast::error_code ec;

    beast::flat_buffer buffer;

    send_lambda lambda{_stream, close_conn, ec, yield};

    for(;;)
    {
        _stream.expires_after(std::chrono::seconds(1));

        // Process request
        http::request<http::string_body> req;
        http::async_read(_stream, buffer, req, yield[ec]);
        if(ec == http::error::end_of_stream)
            break;
        if (ec) {
            std::cout << "--- END ---" << std::endl;
            return fail(ec, "read");
        }

        handle_request(std::move(req), lambda);
        if (ec) {
            std::cout << "--- END ---" << std::endl;
            return fail(ec, "read");
        }

        if(close_conn)
            break;
    }

    _stream.socket().shutdown(tcp::socket::shutdown_send, ec);
    std::cout << "--- END ---" << std::endl;
}