#pragma once
#include "net_common.h"

namespace rck
{
	namespace net
	{
		template<typename T>
		struct message_header {
			T id{};
			uint32_t size = 0;
		};

		template<typename T>
		struct message {
			message_header<T> header{};
			std::vector<uint8_t> body;

			sizet_t size() const {
				return sizeof(message_header<T>) + body.size();
			}
			friend std::ostream& operator<<(std::ostream& os, const message<T>& msg) {
				os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
				return os;
			}
			template<typename DataType>
			friend message<T>& operator<< (message<T>& msg, const DataType& data) {

			}
		};
	}
}