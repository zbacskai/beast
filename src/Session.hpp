#ifndef Session_hpp
#define Session_hpp

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>

namespace beast = boost::beast;   // from <boost/beast>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>

class Session {
    private:
        beast::tcp_stream _stream;
    public:
        Session(tcp::socket&& socket);
        void operator()(net::yield_context yield);
};

#endif