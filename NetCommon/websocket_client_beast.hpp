#pragma once
#include <boost\beast\core.hpp>
#include <boost\beast\websocket.hpp>
#include <boost/asio/spawn.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void fail(beast::error_code ec, const char* what)
{
	std::cerr << what << ":" << ec.message() << std::endl;
}
void do_session(std::string host, std::string& port, std::string& text , net::io_context& ioc, net::yield_context  yield)
{
	beast::error_code ec;

	tcp::resolver resolver(ioc);
	websocket::stream<beast::tcp_stream> ws(ioc);

	auto const results = resolver.async_resolve(host, port, yield[ec]);
	if (ec)
		return fail(ec, "resolve");

	beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));

	auto ep = beast::get_lowest_layer(ws).async_connect(results, yield[ec]);
	if (ec)
		return fail(ec, "connect");

	host += ':' + std::to_string(ep.port());

	beast::get_lowest_layer(ws).expires_never();

	ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
	ws.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
		req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
	}));

	ws.async_handshake(host, "/", yield[ec]);
	if (ec)
		return fail(ec, "handshake");

	ws.async_write(net::buffer(std::string(text)), yield[ec]);
	if (ec)
		return fail(ec, "write");

	beast::flat_buffer buffer;
	ws.async_read(buffer, yield[ec]);
	if (ec)
		return fail(ec, "read");

	// Close the WebSocket connection
	ws.async_close(websocket::close_code::normal, yield[ec]);
	if (ec)
		return fail(ec, "close");

	// If we get here then the connection is closed gracefully

	// The make_printable() function helps print a ConstBufferSequence
	std::cout << beast::make_printable(buffer.data()) << std::endl;

}
int websocket_client_init(const std::string& host, const std::string& port, const std::string& text)
{
	/*if (argc != 4)
	{
		std::cerr <<
			"Usage: websocket-client-coro <host> <port> <text>\n" <<
			"Example:\n" <<
			"    websocket-client-coro echo.websocket.org 80 \"Hello, world!\"\n";
		return EXIT_FAILURE;
	}*/

	net::io_context ioc;

	boost::asio::spawn(ioc, std::bind(&do_session, host, port, text, std::ref(ioc), std::placeholders::_1));

	ioc.run();

	return 0;
}
