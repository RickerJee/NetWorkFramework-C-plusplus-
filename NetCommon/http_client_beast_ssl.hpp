#pragma once

//#ifdef _WIN32
//#define _WIN32_WINNT 0x0A00
//#endif // _WIN32


#include "root_certificates.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/spawn.hpp>
//#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


void fail(beast::error_code ec, const char* what)
{
	std::cerr << what << ":" << ec.message() << std::endl;
}
void do_session(std::string& host, std::string& port, std::string& target, int version, net::io_context& ioc, ssl::context& ctx, net::yield_context  yield)
{
	beast::error_code ec;

	tcp::resolver resolver(ioc);

	beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

	if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
	{
		ec.assign(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
		std::cerr << ec.message() << std::endl;
		return;
	}

	const auto results = resolver.async_resolve(host, port, yield[ec]);

	if (ec)
	{
		return fail(ec, "resolve");
	}

	beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

	get_lowest_layer(stream).async_connect(results, yield[ec]);
	if (ec)
		return fail(ec, "connect");

	beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
	stream.async_handshake(ssl::stream_base::client, yield[ec]);
	if (ec)
		return fail(ec, "handshake");

	http::request<http::string_body> req{ http::verb::get,target,version };
	req.set(http::field::host, host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

	http::async_write(stream, req, yield[ec]);
	if (ec)
		return fail(ec, "write");

	beast::flat_buffer b;
	http::response<http::dynamic_body> res;
	http::async_read(stream, b, res, yield[ec]);

	if (ec)
		return fail(ec, "read");

	std::cout << res << std::endl;

	beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
	stream.async_shutdown(yield[ec]);

	if (ec == net::error::eof)
	{
		// Rationale:
		// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
		ec = {};
	}
	if (ec)
		return fail(ec, "shutdown");

}
int http_client_ssl_init(const std::string& host, const std::string& port, const std::string& target, const std::string& szversioin)
{
	/*if (argc != 4 && argc != 5)
	{
		std::cerr <<
			"Usage: http-client-coro-ssl <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
			"Example:\n" <<
			"    http-client-coro-ssl www.example.com 443 /\n" <<
			"    http-client-coro-ssl www.example.com 443 / 1.0\n";
		return EXIT_FAILURE;
	}*/

	int version = (szversioin != "" && !std::strcmp("1.0", szversioin.c_str()) ? 10 : 11);

	// The io_context is required for all I/O
	net::io_context ioc;

	// The SSL context is required, and holds certificates
	ssl::context ctx{ ssl::context::tlsv12_client };

	// This holds the root certificate used for verification
	load_root_certificates(ctx);

	// Verify the remote server's certificate
	ctx.set_verify_mode(ssl::verify_peer);

	// Launch the asynchronous operation
	boost::asio::spawn(ioc, std::bind(
		&do_session,
		std::string(host),
		std::string(port),
		std::string(target),
		version,
		std::ref(ioc),
		std::ref(ctx),
		std::placeholders::_1));

	// Run the I/O service. The call will return when
	// the get operation is complete.
	ioc.run();

	return 0;
}
