#include "networking.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

// Networking struct to encapsulate SOCKET
struct Networking {
    SOCKET socket = INVALID_SOCKET;
};

// Global instance of Networking
Networking g_networking;

// Function to get the local IP address
std::string getLocalIP() {
    // Initialize Winsock
    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        return "Unknown IP";
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        WSACleanup();
        return "Unknown IP";
    }

    struct addrinfo hints, * res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        WSACleanup();
        return "Unknown IP";
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in*)res->ai_addr)->sin_addr, ip, sizeof(ip));
    freeaddrinfo(res);

    WSACleanup(); // Cleanup Winsock
    return std::string(ip);
}

// Function to receive messages
void receiveMessages(std::function<void(const std::string&)> onMessageReceived) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(g_networking.socket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // Null-terminate the received data
            onMessageReceived(std::string(buffer));
        }
        else if (bytesReceived == 0) {
            onMessageReceived("Connection closed by the other side.");
            break;
        }
        else {
            onMessageReceived("Error receiving data.");
            break;
        }
    }
}

// Start the server in a separate thread
void startServer(const std::string& hostCharacter, std::function<void(const std::string&)> onMessageReceived) {
    std::thread([hostCharacter, onMessageReceived]() {
        WSADATA wsData;
        WSAStartup(MAKEWORD(2, 2), &wsData);

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(20000);

        bind(serverSocket, (sockaddr*)&address, sizeof(address));
        listen(serverSocket, SOMAXCONN);

        std::cout << "Server is listening..." << std::endl;

        sockaddr_in clientAddress;
        int clientSize = sizeof(clientAddress);
        g_networking.socket = accept(serverSocket, (sockaddr*)&clientAddress, &clientSize);

        // Notify that a connection has been established
        onMessageReceived("Connection established.");

        // Send the host's character choice to the client
        send(g_networking.socket, hostCharacter.c_str(), hostCharacter.size(), 0);

        std::thread(receiveMessages, onMessageReceived).detach();
        }).detach();
}

// Start the client
void startClient(const std::string& serverIP, std::function<void(const std::string&)> onMessageReceived) {
    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);

    g_networking.socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(20000);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr);

    connect(g_networking.socket, (sockaddr*)&serverAddress, sizeof(serverAddress));

    // Notify that a connection has been established
    onMessageReceived("Connection established.");

    std::thread(receiveMessages, onMessageReceived).detach();
}

// Send a message
void sendMessage(const std::string& message) {
    send(g_networking.socket, message.c_str(), message.size(), 0);
}