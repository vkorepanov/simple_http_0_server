/*
 * server.cpp
 * Copyright (C) 2017 Korepanov Vyacheslav <real93@live.ru>
 *
 * Distributed under terms of the MIT license.
 */

#include "server.h"

#include <algorithm>
#include <arpa/inet.h>
#include <future>
#include <fstream>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <signal.h>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "boost_parser/request_parser.hpp"
#include "common.h"
#include "optional.h"

namespace {
template <typename S>
inline S& operator<<(S& s, const http::header& h) {
    return s << h.name << ": " << h.value;
}

template <typename S, typename T>
inline S& operator<<(S& s, const std::vector<T>& v) {
    std::for_each(v.begin(), v.end(), [&s](const T& t) {
        s << t << ' ';
    });

    return s << std::endl;
}

template <typename S>
inline S& operator<<(S& s, const http::request& request) {
    return s << request.method << "HTTP/" << request.http_version_major << '.'
      << request.http_version_minor << ' ' << request.headers;
}

inline sockaddr* sockaddrCast(sockaddr_in* v) {
    return reinterpret_cast<sockaddr*>(v);
}

void shutdownSock(int sock) {
    shutdown(sock, SHUT_RDWR);
}

optional<std::string> parseUri(const std::string& uri) {
    if (uri.empty() || uri.front() != '/'
            || uri.find("..") != std::string::npos) {
        std::cerr << "Invalid URI \"" << uri << '\"' << std::endl;
        return nothing<std::string>();
    }

    std::string result;
    result.reserve(uri.size());
    for (auto i = std::string::size_type(1); i < uri.size(); ++i) {
        const auto& ch = uri[i];
        if (ch == '%') {
            std::cerr << "Internationalized URI is not supported" << std::endl;
            return nothing<std::string>();
        }

        static const std::set<char> STOP_SYMBOLS = { '&', ';', '?' };
        static const auto END_IT  = STOP_SYMBOLS.end();
        if (STOP_SYMBOLS.find(ch) != END_IT) {
            break;
        }
        result += ch;
    }

    if (result.back() == '/')
        result += "index.html";
    return just(std::move(result));
}

static constexpr char CRLF[] = {'\r', '\n'};

void replyContents(int clientSocket, int status, const std::string& statusStr
    , const std::vector<http::header>& headers, const std::string& content) {

    std::stringstream writeStringStream;
    writeStringStream << "HTTP/1.0 " << status << ' ' << statusStr << CRLF;
    for (const auto& header: headers) {
        writeStringStream << header.name << ": " << header.value << CRLF;
    }
    writeStringStream << CRLF << content;

    const auto& writeStr = writeStringStream.str();
    std::cout << "Reply str: " << writeStr << std::endl;
    size_t writed = 0;
    while (writed < writeStr.size()) {
        writed = callStdlibFunc([]{}, write, clientSocket
                              , writeStr.c_str() + writed
                              , writeStr.size() - writed);
    }
}

std::vector<http::header> getHeaders(size_t contentSize) {
    std::vector<http::header> headers;
    headers.push_back({"Content-Length", std::to_string(contentSize)});
    headers.push_back({"Content-Type", "text/html"});
    return headers;
}

void replyNotFound(int clientSocket) {
    static const std::string NOT_FOUND_CONTEXT = "Not found";
    replyContents(clientSocket, 404, "Not found"
                , getHeaders(NOT_FOUND_CONTEXT.size()), NOT_FOUND_CONTEXT);
}

void reply(int clientSocket, const http::request& request
         , const std::string& dir) {
    if (request.method != "GET") {
        std::cerr << "Method " << request.method
            << " is not supported" << std::endl;
        replyNotFound(clientSocket);
        return shutdownSock(clientSocket);
    }

    auto maybeRequestFile = parseUri(request.uri);
    if (!maybeRequestFile) {
        std::cerr << "Can't parse URI: " << request.uri << std::endl;
        return replyNotFound(clientSocket);
    }

    const auto requestFile = dir + maybeRequestFile.take();
    std::string buffer;

    {
        static const auto FD_DELETER = [](FILE* fd) {
            if (fd) fclose(fd); };
        const auto fd = std::unique_ptr<FILE, decltype(FD_DELETER)>(
                fopen(requestFile.c_str(), "r"), FD_DELETER);
        if (fd <= 0) {
            std::cerr << "Can't open file: " << requestFile << std::endl;
            return replyNotFound(clientSocket);
        }

        constexpr size_t SIZE = 65534;
        char bufStr[SIZE + 1] = {0};
        while (fread(bufStr, sizeof(char), SIZE - 1, fd.get())) {
            bufStr[SIZE] = 0;
            buffer += bufStr;
        }
    }

    replyContents(clientSocket, 200, "OK", getHeaders(buffer.size()), buffer);
}

void handleConnection(int clientSocket, const std::string& dir) {
    constexpr size_t BUF_SIZE = 65535;
    char buffer[BUF_SIZE];
    bzero(buffer, BUF_SIZE);
    while (true) {
        const auto bytesRead = callStdlibFunc([]{}, read, clientSocket
                , buffer, BUF_SIZE);
        if (bytesRead <= 0) {
            std::cerr << "Can't read request from client" << std::endl;
            break;
        }

        http::request request;
        http::request_parser parser;
        const auto parseResult = std::get<0>(parser.parse(request, buffer
                                           , buffer + BUF_SIZE));
        if (parseResult == http::request_parser::good) {
            std::cout << "Request was accepted: " << request << std::endl;
            return reply(clientSocket, request, dir);
        }
        else if (parseResult == http::request_parser::bad) {
            std::cerr << "Bad request: " << std::endl << buffer << std::endl;
            break;
        }
    }

    replyNotFound(clientSocket);
    shutdownSock(clientSocket);
}
}

namespace http {
std::mutex server::instancesMutex;
std::set<server*> server::serverInstances;

server::server(const std::string& address, short port
             , const std::string& rootDir)
    : m_rootDir(rootDir.empty()
                ? "./"
                : rootDir.back() != '/'
                    ? rootDir + '/'
                    : rootDir)
{
    signal(SIGINT, server::sigHandler);
    const auto shutdownOnError = [this] {
        m_socket != INVALID_SOCK ? void(shutdownSock(m_socket)) : void();
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

    {
        std::lock_guard<std::mutex> lock(instancesMutex);
        serverInstances.insert(this);
    }
    m_thread = std::thread(&server::acceptConnections, this);
}

server::~server() {
    shutdownSock(m_socket);
    {
        std::lock_guard<std::mutex> lock(instancesMutex);
        serverInstances.erase(this);
    }
    joinToAcceptorThread();
}

void server::sigHandler(int sig) {
    if (sig == SIGINT) {
        std::lock_guard<std::mutex> lock(instancesMutex);
        for (server* obj: serverInstances)
            obj->sigintHandler();
    }
}

void server::sigintHandler() {
    shutdownSock(m_socket);
}

void server::acceptConnections() const {
    sockaddr_in sock;
    socklen_t sockSize = sizeof(sockaddr_in);
    while (true) {
        const auto clientSocket = callStdlibFunc([] {/*nothing*/}
            , accept, m_socket, sockaddrCast(&sock), &sockSize);
        if (clientSocket < 0 && clientSocket != EAGAIN)
            break;

        std::cout << "Connected client: " << sock.sin_addr.s_addr << std::endl;
        std::async(std::launch::async, handleConnection
                 , clientSocket, m_rootDir);
    }
}

void server::joinToAcceptorThread() {
    if (m_thread.joinable())
        m_thread.join();
}
} // namespace http

