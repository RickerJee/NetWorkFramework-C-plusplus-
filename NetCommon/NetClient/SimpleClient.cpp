// NetClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "rck_net.h"

enum class CustomerTypes :uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

class CustomerClient : public rck::net::client_interface<CustomerTypes>
{
public:
    void PingServer()
    {
        rck::net::message<CustomerTypes> msg;
        msg.header.id = CustomerTypes::ServerPing;

        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

        msg << timeNow;
        Send(msg);
    }

    void MessageAll()
    {
        rck::net::message<CustomerTypes> msg;
        msg.header.id = CustomerTypes::MessageAll;
        Send(msg);
    }
};

int main()
{
    CustomerClient c;
    c.Connect("127.0.0.1", 60000);
    bool bQuit = false;

    bool key[3] = { false,false,false };
    bool old_key[3] = { false,false,false };

    while (!bQuit) {
        
        if (GetForegroundWindow() == GetConsoleWindow())
        {
            key[0] = GetAsyncKeyState('1') & 0x8000;
            key[1] = GetAsyncKeyState('2') & 0x8000;
            key[2] = GetAsyncKeyState('3') & 0x8000;
        }

        if (key[0] && !old_key[0]) c.PingServer();
        if (key[1] && !old_key[1]) c.MessageAll();

        if (key[2] && !old_key[2]) bQuit = true;

        for (int i = 0; i < 3; ++i) old_key[i] = key[i];

        if (c.IsConnected())
        {
            if (!c.Incoming().empty())
            {
                auto msg = c.Incoming().pop_front().msg;

                switch (msg.header.id)
                {
                case CustomerTypes::ServerAccept:
                {
                    std::cout << "Server Accept Connection\n";
                    break;
                }
                case CustomerTypes::ServerPing:
                {
                    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point timeThen;
                    msg >> timeThen;
                    std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
                    break;
                }
                case CustomerTypes::ServerMessage:
                {
                    uint32_t clientID;
                    msg >> clientID;
                    std::cout << "Hello from [" << clientID << "]\n";
                    break;
                }

                default:
                    break;
                }
            }
        }
        else
        {
            std::cout << "Server Down\n";
            bQuit = true;
        }
    }
    return 0;
}

