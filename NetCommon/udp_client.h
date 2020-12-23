#pragma once
#include <string>
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <iostream>
using asio::ip::udp;

bool nUDP_connect_server(const std::string& host, uint16_t port)
{
	try
	{
		asio::io_context io_context;

		udp::resolver resolver(io_context);
		udp::endpoint sender_endpoint = *resolver.resolve(udp::v4(), host, std::to_string(port)).begin();

		udp::socket socket(io_context);
		socket.open(udp::v4());

		std::array<char, 1> send_buf = { {0} };
		socket.send_to(asio::buffer(send_buf), sender_endpoint);

		std::array<char, 128> recv_buf;

		udp::endpoint receive_point;
		size_t len = socket.receive_from(asio::buffer(recv_buf), receive_point);

		std::cout.write(recv_buf.data(), len);

		return true;
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return false;
}
