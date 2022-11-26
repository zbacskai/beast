#include <HttpServer.hpp>

HttpServer::ServeConfig getServerConfig(const int argc, char* const argv[])
{
    if (argc != 4) {
        std::stringstream ss;
        ss <<
            "Usage: " << argv[0] << " <address> <port> <threads>\n" <<
            "Example:\n" <<
            "    " << argv[0] << " 0.0.0.0 8080 1\n";
        throw std::invalid_argument(ss.str()); 
    }

    return {
        net::ip::make_address(argv[1]), static_cast<unsigned short>(std::atoi(argv[2])), std::max<int>(1, std::atoi(argv[3]))
    };
}

int main(int argc, char* argv[])
{
    try {
        HttpServer::ServeConfig serverConfig = getServerConfig(argc, argv);

        net::io_context ioc{serverConfig.threads};

        HttpServer httpServer(ioc, serverConfig.address, serverConfig.port);

        boost::asio::spawn(ioc, std::bind(httpServer, std::placeholders::_1));

        std::vector<std::thread> v;
        v.reserve(serverConfig.threads - 1);
        for (auto i = serverConfig.threads - 1; i >0; --i)
        v.emplace_back([&ioc]{ ioc.run();});

        ioc.run();
    }
    catch (const std::invalid_argument& invalidConfig) {
        std::cerr << invalidConfig.what() << std::endl;
        return -1;
    }

    return 0;
}