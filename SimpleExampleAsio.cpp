// SimpleExample.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <string>
#include <vector>
#include <chrono>

std::vector<char> vBuffer(1 * 1024);
void GrabSomeData(asio::ip::tcp::socket& socket) {
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()), [&](std::error_code ec,std::size_t length) {
		if (!ec) {
			std::cout << "\n\nRead " << length << " bytes\n\n";
			for (int i = 0; i < length; ++i) {
				std::cout << vBuffer[i];

				GrabSomeData(socket);
			}
		}
		});
}
int main()
{
	asio::error_code ec;

	asio::io_context context;

	asio::io_context::work idleWork(context);
	std::thread thrContext = std::thread([&]() {context.run(); });
	
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("74.125.24.105"), 80); //google.com

	asio::ip::tcp::socket socket(context);

	socket.connect(endpoint, ec);

	if (!ec)
		std::cout << "connected!" << std::endl;
	else
	{
		std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
	}

	if (socket.is_open()) {

		GrabSomeData(socket);

		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host:example.com\r\n"
			"Connection:close\r\n\r\n";

		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);
		

		////std::this_thread::sleep_for(std::chrono::milliseconds(200));
		//socket.wait(socket.wait_read);

		//size_t bytes = socket.available();

		//std::cout << "Bytes Avaiable: " << bytes << std::endl;

		//if (bytes > 0)
		//{
		//	std::vector<char> vBuffer(bytes);
		//	socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec);

		//	for (auto c : vBuffer) {
		//		std::cout << c;
		//	}
		//}

		std::this_thread::sleep_for(std::chrono::milliseconds(2000));

		socket.close();

		context.stop();
		if (thrContext.joinable())thrContext.join();

	}
	getchar();


	return 0;

}

