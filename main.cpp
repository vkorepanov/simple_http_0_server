/*
 * main.cpp
 * Copyright (C) 2017 Korepanov Vyacheslav <real93@live.ru>
 *
 * Distributed under terms of the MIT license.
 */
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include "common.h"
#include "server.h"

int main(int argc, char **argv) {
    static const std::string optstring("h:p:d:");

    int c{0};
    std::string address;
    std::string port;
    std::string rootDirectory;
    while ( (c = getopt(argc, argv, optstring.c_str())) != -1) {
        switch (c) {
            case 'h':
                address = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'd':
                rootDirectory = optarg;
                break;
            case '?':
            {
                const auto it = optstring.find(optopt);
                if (it != std::string::npos) {
                    std::cerr << "Option \"" << static_cast<char>(optopt)
                              << "\" requires an argument." << std::endl;
                    break;
                }
                std::cerr << "Unrecognized option \""
                          << static_cast<char>(optopt) << '\"' << std::endl;
                break;
            }
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (port.empty() || address.empty()) {
        std::cerr << "Usage: " << argv[0]
                  << " -h <IP> -p <port> -d <directory>" << std::endl;
        exit(EXIT_FAILURE);
    }

     callStdlibFunc(abort, daemon, 1, 1);

    std::cout << "[" << getpid() << "]"           << std::endl
        << "address = "          << address       << std::endl
        << "port = "             << port          << std::endl
        << "root directory = "   << rootDirectory << std::endl;

    try {
        http::server server(address, getFromStr<short>(port), rootDirectory);
        server.joinToAcceptorThread();
    } catch (std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
}

