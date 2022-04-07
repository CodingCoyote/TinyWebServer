/* ************************************************************************
> File Name:     main.cpp
> Author:        Coyote
> Created Time:  Fri 26 Nov 2021 08:10:21 PM CST
> Description:   
 ************************************************************************/

#include "Server.hpp"
#include <thread>

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "usage: " << basename(argv[0]) <<  "config_filename" << std::endl;
        return 1;
    }

    Server obj(argv[1]);
    
    while (1) {
        obj.handler();
//        std::thread(&Server::handler, obj, connfd).detach();
    }
    
    return 0;
    
}
