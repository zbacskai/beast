#include <HttpServer.hpp>
#include <Session.hpp>

HttpServer::HttpServerException::HttpServerException(const beast::error_code &ec, char const* error_msg) {
    _ss << error_msg << ": " << ec.message();
}

void HttpServer::initialiseSocket(tcp::acceptor& ioAcceptor) const {
    beast::error_code ec;

    // open acceptor
    ioAcceptor.open(_endpoint.protocol(), ec);
    if (ec) throw HttpServerException(ec, "open");

    // allow address reuse
    ioAcceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) throw HttpServerException(ec, "set_option");

    ioAcceptor.bind(_endpoint, ec);
    if (ec) throw HttpServerException(ec, "bind");

    ioAcceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) throw HttpServerException(ec, "listen");
}

HttpServer::HttpServer(
    net::io_context& ioc,
    const net::ip::address& address,
    const unsigned short port
) : _ioc(ioc), _endpoint(tcp::endpoint{address, port}) {}

void HttpServer::operator()(net::yield_context yield) const
{
    try {
        tcp::acceptor acceptor(_ioc);
        initialiseSocket(acceptor);

        beast::error_code ec;

        for (;;)
        {
            tcp::socket socket(_ioc);
            acceptor.async_accept(socket, yield[ec]);

            if (ec) throw HttpServerException(ec, "accept");

            boost::asio::spawn(
                acceptor.get_executor(),
                std::bind(Session(std::move(socket)), std::placeholders::_1));
        }
    }
    catch (HttpServerException& e) {
        std::cerr << e << std::endl;
    }
}

std::ostream& operator<< (std::ostream& os, const HttpServer::HttpServerException& e) {
    os << e._ss.str();
    return os;
}
