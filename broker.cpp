#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "User.h"

#pragma comment(lib, "Ws2_32.lib")

constexpr int PORT = 8175;

std::unordered_map<std::string, User> users;

std::unordered_map<std::string, std::vector<std::string>> topic_subscribers;
std::mutex topic_mutex;

std::unordered_map<std::string, std::vector<std::string>> messages_to_send;
std::mutex message_mutex;

void send_message(const std::string& topic, const std::string& message, const std::string& id, const SOCKET client_socket) {
    std::string full_message = "Topic [" + topic + "]:";
    full_message += message;
    full_message += "\n";

    if (users[id].isConnected()) {
        if (const SOCKET subscriber_socket = users[id].getCurrentSocket(); subscriber_socket != client_socket) {
            send(subscriber_socket, full_message.c_str(), static_cast<int>(full_message.size()), 0);
        }
    } else {
        std::lock_guard lock(message_mutex);
        messages_to_send[full_message].push_back(id);
    }
}

void handle_client(User user) {
    user.setConnected(true);
    const SOCKET client_socket = user.getCurrentSocket();
    char buffer[1024];

    while (true) {
        const int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cout << "Client disconnected.\n";
            closesocket(client_socket);
            break;
        }

        buffer[bytes_received] = '\0';
        std::string message(buffer);
        std::istringstream iss(message);
        std::string command;
        iss >> command;

        std::string topic;
        iss >> topic;

        if (command == "SUBSCRIBE") { //TODO: CREATE A TO_UPPERCASE FUNCTION
            if (!topic.empty()) {
                std::lock_guard lock(topic_mutex);
                topic_subscribers[topic].push_back(user.getId());
                std::cout << "Client subscribed to topic: " << topic << "\n";
            }
        } else if (command == "PUBLISH") {
            std::string content;
            std::getline(iss, content);

            if (!topic.empty()) {
                std::lock_guard lock(topic_mutex);
                if (auto it = topic_subscribers.find(topic); it != topic_subscribers.end()) {
                    for (const std::string& id : it->second) {
                        send_message(topic, content, id, client_socket);
                    }
                }
                std::cout << "Published to topic: " << topic << " -> " << content << "\n";
            }
        } else if (command == "CREATE") {
            //TODO: IMPLEMENT CREATE TOPIC FUNCTION
        } else {
            std::string error_msg = "Unknown command.\n";
            send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.size()), 0);
        }

        user.setConnected(false);
        closesocket(client_socket);
    }
}

void authenticate_client(const SOCKET client_socket) {
    //TODO: AUTHENTICATE CLIENT VIA DIGITAL CERTIFICATE
    const bool authenticated = true;
    const std::string certificate = "12345";

    if (authenticated) {
        if (const auto it = users.find(certificate); it != users.end()) {
            handle_client(it->second);
        } else {
            const User user(certificate, client_socket);
            users[certificate] = user;
            handle_client(user);
        }

    } else {
        std::string error_msg = "Authentication failed.\n";
        send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.size()), 0);
        closesocket(client_socket);
    }
}

[[noreturn]] void start_server() {
    WSADATA wsaData;
    if (const int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData); wsaInit != 0) {
        std::cerr << "WSAStartup failed with error: " << wsaInit << "\n";
        exit(EXIT_FAILURE);
    }

    const SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, reinterpret_cast<SOCKADDR *>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    std::cout << "Broker listening on port " << PORT << "...\n";

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed.\n";
            continue;
        }

        std::cout << "New client connected.\n";
        std::thread(authenticate_client, client_socket).detach();

        std::lock_guard lock(message_mutex);
        for (const auto& [message, pending_users] : messages_to_send) {
            for (int i = 0; i < messages_to_send[message].size(); ++i) {
                if (const std::string id = messages_to_send[message].at(i); users[id].isConnected()) {
                    if (const SOCKET subscriber_socket = users[id].getCurrentSocket()) {
                        send(subscriber_socket, message.c_str(), static_cast<int>(message.size()), 0);
                        messages_to_send[message].erase(messages_to_send[message].begin() + i);
                    }
                }
            }
        }
    }
}

int main() {
    start_server();
}