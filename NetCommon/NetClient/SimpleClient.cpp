// NetClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "rck_net.h"

enum class CustomerMsgTypes : uint32_t
{
    FireBullet,
    MovePlayer
};

int main()
{
    rck::net::message<CustomerMsgTypes> msg;
    msg.header.id = CustomerMsgTypes::FireBullet;

    int a = 1;
    bool b = true;
    float c = 3.1415;

    struct
    {
        float x=4.5f;
        float y=4.6f;
    }d[5];

    //msg << a << b << c << d;
    msg << d;

    a = 99;
    b = false;
    c = 90.0f;

    //msg >> d >> c >> b >> a;
    msg >> d;


    std::cout << "Hello World!\n";
}

