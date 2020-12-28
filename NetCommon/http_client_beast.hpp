#pragma once

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif // _WIN32

#include <boost\beast\core.hpp>
#include <boost\beast\http.hpp>
#include <boost\beast\version.hpp>
#include <boost\asio\spawn.hpp>
#include <boost\asio\ip\tcp.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
void fail(beast::error_code ec, const char* what)
{
	std::cerr << what << ":" << ec.message() << std::endl;
}

void do_session(std::string& host, std::string& port, std::string& target, int version, net::io_context& ioc, net::yield_context  yield)
{
	beast::error_code ec;

	tcp::resolver resolver(ioc);
	beast::tcp_stream stream(ioc);

	auto const results = resolver.async_resolve(host, port, yield[ec]);
	if (ec)
		return fail(ec, "resolve");

	stream.expires_after(std::chrono::seconds(30));
	stream.async_connect(results, yield[ec]);
	if (ec)
		return fail(ec, "connect");

	http::request<http::string_body> req{ http::verb::get,target,version };
	req.set(http::field::host, host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	stream.expires_after(std::chrono::seconds(30));
	http::async_write(stream, req, yield[ec]);
	if (ec)
		return fail(ec, "write");

	beast::flat_buffer b;
	http::response<http::dynamic_body> res;
	http::async_read(stream, b, res, yield[ec]);

	if (ec)
		return fail(ec, "read");

	std::cout << res << std::endl;

	stream.socket().shutdown(tcp::socket::shutdown_both, ec);

	if (ec && ec != beast::errc::not_connected)
		return fail(ec, "shutdown");

}

int http_client_init(const std::string& host, const std::string& port, const std::string& target, const std::string& szversioin)
{
	try
	{
		/*if (argc != 4 && argc != 5)
		{
		std::cerr <<
		"Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
		"Example:\n" <<
		"    http-client-sync www.example.com 80 /\n" <<
		"    http-client-sync www.example.com 80 / 1.0\n";
		}*/


		int version = (szversioin != "" && !std::strcmp("1.0", szversioin.c_str()) ? 10 : 11);

		net::io_context ioc;

		boost::asio::spawn(ioc, std::bind(&do_session, host, port, target, version, std::ref(ioc), std::placeholders::_1));

		ioc.run();

	}
	catch (std::exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}
	return 0;
}
