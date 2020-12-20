// NetServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <rck_net.h>

enum class CustomerTypes :uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};
class CustomerServer :public rck::net::server_interface<CustomerTypes>
{
public:
    CustomerServer(uint16_t nPort) :rck::net::server_interface<CustomerTypes>(nPort)
    {
       
    }

protected:
    virtual bool OnClientConnect(std::shared_ptr<rck::net:: connection<CustomerTypes>> client)
    {
        rck::net::message<CustomerTypes> msg;
        msg.header.id = CustomerTypes::ServerAccept;

        client->Send(msg);

        return true;
    }
    virtual void OnClientDisconnect(std::shared_ptr<rck::net::connection<CustomerTypes>> client)
    {
        std::cout << "Removeing client [" << client->GetID() << "]\n";
    }
    virtual void OnMessage(std::shared_ptr<rck::net::connection<CustomerTypes>> client, rck::net::message<CustomerTypes>& msg)
    {
        switch (msg.header.id) {
            case CustomerTypes::ServerPing:
            {
                std::cout << "[" << client->GetID() << "]: Server Ping\n";
                client->Send(msg);
                break;
            }
            case CustomerTypes::MessageAll:
            {
                std::cout << "[" << client->GetID() << "]: Message All\n";

                rck::net::message<CustomerTypes> msg;
                msg.header.id = CustomerTypes::ServerMessage;
                msg << client->GetID();
                MessageAllClients(msg, client);
            }
            break;
        }
    }
};
int main()
{
    CustomerServer server(60000);
    server.Start();

    while (1)
    {
        server.Upate(-1,true);
    }

    return 0;
}
