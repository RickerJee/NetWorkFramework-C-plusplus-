#pragma once
#include <string>
#include <ctime>
#include <iostream>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

using asio::ip::udp;

std::string make_daytime_string()
{
	using namespace std; // For time_t, time and ctime;
	time_t now = time(0);

	char cBuf[128] = { '\0' };
	ctime_s(cBuf,128,&now);

	return string(cBuf);
}

bool nUDP_InitServer(uint16_t port)
{
	try
	{
		asio::io_context io_context;
		udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

		for (;;)
		{
			std::array<char, 1> recv_buf;
			udp::endpoint remote_endpoint;
			socket.receive_from(asio::buffer(recv_buf), remote_endpoint);

			std::string message = make_daytime_string();
			std::error_code ignore_code;
			socket.send_to(asio::buffer(message), remote_endpoint, 0, ignore_code);
		}
		return true;
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return false;
}
