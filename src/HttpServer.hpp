#ifndef HttpServer_hpp
#define HttpServer_hpp

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>

#include <sstream>
#include <iostream>

namespace net = boost::asio;          // from <boost/asio>
using tcp = boost::asio::ip::tcp;     // from <boost/asio/ip/tcp.hpp>
namespace beast = boost::beast;       // from <boost/beast.hpp>

class HttpServer {
    public:

        class HttpServerException {
            protected:
                std::stringstream _ss;
            public:
                HttpServerException(const beast::error_code &ec, char const *what);
                friend std::ostream& operator<< (std::ostream& os, const HttpServerException& e);
        };

        struct ServeConfig {
            const net::ip::address address;
            const unsigned short port;
            const int threads;
        };

    protected:
        net::io_context& _ioc;
        tcp::endpoint _endpoint;

        void initialiseSocket(tcp::acceptor& ioAcceptor) const;

    public:

        HttpServer(
            net::io_context& ioc,
            const net::ip::address& address,
            const unsigned short port);

        void operator()(net::yield_context yield) const;
};

std::ostream& operator<< (std::ostream &os, const HttpServer::HttpServerException& e);

#endif