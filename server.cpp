/*
 * server.cpp
 * Copyright (C) 2017 Korepanov Vyacheslav <real93@live.ru>
 *
 * Distributed under terms of the MIT license.
 */

#include "server.h"

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

namespace {
inline sockaddr* sockaddrCast(sockaddr_in* v) {
    return reinterpret_cast<sockaddr*>(v);
}
}

namespace http {
server::server(const std::string& address, short port
             , const std::string& rootDir)
    : m_rootDir(rootDir.empty() ? "." : rootDir)
{
    const auto shutdownOnError = [this] {
        m_socket != INVALID_SOCK ? void(shutdown(m_socket, SHUT_RDWR)) : void();
        throw std::runtime_error("Can't construct a server");
    };

    m_socket = callStdlibFunc(shutdownOnError, socket, AF_INET, SOCK_STREAM, 0);

    sockaddr_in sock;
    bzero(&sock, sizeof(sock));
    sock.sin_family = AF_INET;
    if (inet_aton(address.c_str(), &sock.sin_addr) == 0) {
        std::cerr << "Не удалось преобразовать адрес \""
                  << address << "\" в IP." << std::endl;
    }

    sock.sin_port = htons(port);
    callStdlibFunc(shutdownOnError, bind, m_socket
                 , sockaddrCast(&sock), sizeof(sockaddr_in));

    callStdlibFunc(shutdownOnError, listen, m_socket, SOMAXCONN);

    m_thread = std::thread(&server::acceptConnections, this);
}

server::~server() {
    shutdown(m_socket, SHUT_RDWR);
    joinToAcceptorThread();
}

void server::acceptConnections() {
    sockaddr_in sock;
    socklen_t sockSize = sizeof(sockaddr_in);
    bzero(&sock, sockSize);
    while (true) {
        const auto clientSocket = callStdlibFunc([] {/*nothing*/}
            , accept, m_socket, sockaddrCast(&sock), &sockSize);
    }
}

void server::joinToAcceptorThread() {
    if (m_thread.joinable())
        m_thread.join();
}
} // namespace http

