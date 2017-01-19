/*
 * server.h
 * Copyright (C) 2017 Korepanov Vyacheslav <real93@live.ru>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <thread>

namespace http {
/**
 * @brief Simple HTTP 1.0 server class.
 * It listen for clients on a port and asynchronous accept connections.
 * It is possible to join to server thread which accept connections.
 */
class server {
    public:
        /**
         * @brief Construct a server.
         *
         * @param address - Internet address.
         * @param port - Connection port.
         * @param rootDir - Root directory. Server will send requested files from it.
         */
        server(const std::string& address, short port
             , const std::string& rootDir) noexcept(false);
        ~server();

        /**
         * @brief Join to server thread.
         */
        void joinToAcceptorThread();

    private:
        void acceptConnections();

    private:
        static constexpr int INVALID_SOCK = -1;
        int m_socket = INVALID_SOCK;
        std::string m_rootDir;
        std::thread m_thread;
};
} // namespace http

#endif /* !SERVER_H */
