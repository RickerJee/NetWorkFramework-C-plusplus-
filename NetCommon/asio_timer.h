#pragma once

#include <chrono>
#include <iostream>
#define ASIO_STANDALONE
#include <asio.hpp>

class printer
{
public:
	printer(asio::io_context& io):timer_(io,std::chrono::seconds(1))
	{
		timer_.async_wait(std::bind(&printer::print, this));
	}
	~printer()
	{
		std::cout << "Final count is " << count_ << std::endl;
	}
	void print()
	{
		if (count_ < 5)
		{
			std::cout << count_ << std::endl;
			++count_;

			timer_.expires_at(timer_.expiry() + std::chrono::seconds(1));
			timer_.async_wait(std::bind(&printer::print, this));

		}
	}
private:
	asio::steady_timer timer_;
	int count_;
};

void Timer_Init()
{
	asio::io_context io;
	printer p(io);
	io.run();

}
