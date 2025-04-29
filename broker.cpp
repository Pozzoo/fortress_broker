#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

constexpr int PORT = 8175;

std::unordered_map<std::string, std::vector<SOCKET>> topic_subscribers; //TODO: CHANGE THIS TO AN OBJECT ARRAY THAT SAVES THE CLIENTS ID
std::mutex topic_mutex;

//TODO: CREATE A PENDING MESSAGE LIST (FOR EACH TOPIC REFERENCING ITS CLIENTS)

void handle_client(const SOCKET client_socket) {
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

        switch (command) {
            case "SUBSCRIBE":
                if (!topic.empty()) {
                    std::lock_guard lock(topic_mutex);
                    topic_subscribers[topic].push_back(client_socket);
                    std::cout << "Client subscribed to topic: " << topic << "\n";
                }
                break;

            case "PUBLISH":
                std::string content;
                std::getline(iss, content);

                if (!topic.empty()) {
                    std::lock_guard lock(topic_mutex);
                    if (auto it = topic_subscribers.find(topic); it != topic_subscribers.end()) {
                        for (const SOCKET subscriber_socket : it->second) {
                            if (subscriber_socket != client_socket) {
                                std::string full_message = "Topic [" + topic + "]:";
                                full_message += content;
                                full_message += "\n";

                                send(subscriber_socket, full_message.c_str(), static_cast<int>(full_message.size()), 0);
                            }
                        }
                    }
                    std::cout << "Published to topic: " << topic << " ->" << content << "\n";
                }
                break;

            //TODO: ADD A CREATE COMMAND

            default:
                std::string error_msg = "Unknown command.\n";
                send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.size()), 0);
                break;
        }

        //TODO: HANDLE CLIENT DISCONNECT (CLOSE THE SOCKET AND KEEP THE ID)
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
        std::thread(handle_client, client_socket).detach();
    }
}

int main() {
    start_server();
}
