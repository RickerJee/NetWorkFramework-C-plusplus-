#pragma once
#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace rck {
	namespace net
	{
		template<typename T>
		class server_interface;

		template<typename T>
		class connection :public std::enable_shared_from_this<connection<T>>
		{
		public:
			enum class owner {
				server,
				client
			};

			connection(owner parent,asio::io_context& asioContext,asio::ip::tcp::socket socket,tsqueue<owned_message<T>>& qIn)
				:m_asioContext(asioContext),m_socket(std::move(socket)),m_qMessageIn(qIn)
			{
				m_nOwnerType = parent;

				if (m_nOwnerType == owner::server)
				{
					m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
					m_nHandshakeCheck = scramble(m_nHandshakeOut);
				}
				else
				{
					m_nHandshakeIn = 0;
					m_nHandshakeOut = 0;
				}
			}
			virtual ~connection(){};
			uint32_t GetID() const {
				return id;
			}
		public:
			void ConnectToClient(rck::net::server_interface<T>* server, uint32_t uid = 0)
			{
				if (m_nOwnerType == owner::server)
				{
					if (m_socket.is_open())
					{
						id = uid;
						//ReadHeader();

						WriteValidation();
						ReadValidation(server);

					}
				}
			}

			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
			{
				if (m_nOwnerType == owner::client) {
					asio::async_connect(m_socket, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
							if (!ec) {
								//ReadHeader();
								ReadValidation();
							}
							else
							{
								std::cout << "[Client]" <<id<< "connect to server failed." << ec.message() << "\n";
							}
						});
				}
			}
			void Disconnect()
			{
				if (IsConnected())
					asio::post(m_asioContext, [this]() {m_socket.close(); });
			}
			bool IsConnected() const
			{
				return m_socket.is_open();
			}
		public:
			void Send(const message<T>& msg)
			{
				asio::post(m_asioContext,
					[this, msg]() {
						bool bWritingMessage = !m_qMessageOut.empty();

						m_qMessageOut.push_back(msg);

						if (!bWritingMessage)
							WriteHeader();
					});
			}
		private:
			void WriteHeader()
			{
				asio::async_write(m_socket, asio::buffer(&m_qMessageOut.front().header, sizeof(message_header<T>)), 
					[this](std::error_code ec, std::size_t length) {
					if (!ec)
					{
						if (m_qMessageOut.front().body.size() > 0)
						{
							WriteBody();
						}
						else
						{
							m_qMessageOut.pop_front();

							if (!m_qMessageOut.empty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						std::cout << "[" << id << "] Write Header Failed.\n";
						m_socket.close();
					}
					});
			}
			void WriteBody()
			{
				asio::async_write(m_socket, asio::buffer(m_qMessageOut.front().body.data(), m_qMessageOut.front().body.size()), 
					[this](std::error_code ec, std::size_t length) {
					if (!ec)
					{
						m_qMessageOut.pop_front();
						if (!m_qMessageOut.empty())
						{
							WriteHeader();
						}
					}
					else
					{
						std::cout << "[" << id << "] Write Body Failed.\n";
						m_socket.close();
					}
					});
			}
			void ReadHeader()
			{
				asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)), 
					[this](std::error_code ec,std::size_t length) {
						if (!ec)
						{
							if (m_msgTemporaryIn.header.size > 0) {
								m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
								ReadBody();
							}
							else
							{
								AddToIncomingMessageQueue();
							}
						}
						else
						{
							std::cout << "[" << id << "] Read Header Failed.\n";
							m_socket.close();
						}
					});
			}

			void ReadBody()
			{
				asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()), 
					[this](std::error_code ec, std::size_t length) {
					if (!ec)
					{
						AddToIncomingMessageQueue();
					}
					else
					{
						std::cout << "[" << id << "] Read Body Failed.\n";
						m_socket.close();
					}
					});
			}
			
			void AddToIncomingMessageQueue()
			{
				if (m_nOwnerType == owner::server)
					m_qMessageIn.push_back({ this->shared_from_this(),m_msgTemporaryIn });
				else
					m_qMessageIn.push_back({ nullptr,m_msgTemporaryIn });

				ReadHeader();
			}
		public:

			uint64_t scramble(uint64_t nInput)
			{
				uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
				out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0xF0F0F0F0F0F0F0) << 4;
				return out ^ 0xC0DEFACE12345678;

			}

			void WriteValidation()
			{
				asio::async_write(m_socket, asio::buffer(&m_nHandshakeOut, sizeof(uint64_t)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_nOwnerType == owner::client)
								ReadHeader();
						}
						else
						{
							m_socket.close();
						}
					});
			}
			void ReadValidation(rck::net::server_interface<T>* server = nullptr)
			{
				asio::async_read(m_socket, asio::buffer(&m_nHandshakeIn, sizeof(uint64_t)),
					[this, server](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_nOwnerType == owner::server)
							{
								if (m_nHandshakeIn == m_nHandshakeCheck)
								{
									std::cout << "Client Validated\n";
									server->OnClientValidated(this->shared_from_this());

									ReadHeader();
								}
								else
								{
									std::cout << "Client Disconnected(Fail Validation)\n";
									m_socket.close();
								}
							}
							else
							{
								m_nHandshakeOut = scramble(m_nHandshakeIn);
								WriteValidation();
							}
						}
						else
						{
							std::cout << "Client Disconnected (ReadValidation)\n";
							m_socket.close();
						}
					});
			}

		protected:
			asio::ip::tcp::socket m_socket;
			asio::io_context& m_asioContext;

			tsqueue<message<T>> m_qMessageOut;
			tsqueue<owned_message<T>>& m_qMessageIn;
			message<T> m_msgTemporaryIn;

			owner m_nOwnerType = owner::server;

			uint32_t id = 0;

			uint64_t m_nHandshakeOut =0;
			uint64_t m_nHandshakeIn =0;
			uint64_t m_nHandshakeCheck = 0;
		};
	}
}

