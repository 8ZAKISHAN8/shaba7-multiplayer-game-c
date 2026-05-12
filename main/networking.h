#ifndef NETWORKING_H
#define NETWORKING_H

#include <string>
#include <functional>
#include <thread>

// Opaque pointer to hide SOCKET type
struct Networking;

// Function declarations
void startServer(const std::string& hostCharacter, std::function<void(const std::string&)> onMessageReceived);
void startClient(const std::string& serverIP, std::function<void(const std::string&)> onMessageReceived);
void sendMessage(const std::string& message);
std::string getLocalIP(); // Function to get the local IP address

#endif // NETWORKING_H