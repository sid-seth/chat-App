#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

void receiveMessages(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;
        
        std::cout << "\n" << buffer << std::endl;  // Display received messages
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Connected to server!\n";

    // Get username and password from user
    std::cout << "Enter username: ";
    std::string username;
    std::getline(std::cin, username);

    std::cout << "Enter password: ";
    std::string password;
    std::getline(std::cin, password);

    // Send authentication data
    std::string authMessage = username + ":" + password;
    send(clientSocket, authMessage.c_str(), authMessage.length(), 0);

    // Receive authentication response
    char responseBuffer[BUFFER_SIZE];
    memset(responseBuffer, 0, BUFFER_SIZE);
    recv(clientSocket, responseBuffer, BUFFER_SIZE, 0);

    if (std::string(responseBuffer) == "Authentication Failed") {
        std::cout << "Login failed! Closing connection...\n";
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    std::cout << "Login successful! Welcome to the chat, " << username << "!\n";

    std::thread recvThread(receiveMessages, clientSocket);
    recvThread.detach();

    while (true) {
        std::string message;
        std::cout << username << " > ";
        std::getline(std::cin, message);

        // Handle group creation
        if (message.find("/create ") == 0) {
            send(clientSocket, message.c_str(), message.length(), 0);
            std::cout << "Group created: " << message.substr(8) << std::endl;
            continue;
        }

        // Handle group joining
        if (message.find("/join ") == 0) {
            send(clientSocket, message.c_str(), message.length(), 0);
            std::cout << "Joined group: " << message.substr(6) << std::endl;
            continue;
        }

        // Send group messages using #group_name message format
        if (message.find("#") == 0) {
            send(clientSocket, message.c_str(), message.length(), 0);
            std::cout << "Message sent to group " << message.substr(1, message.find(" ") - 1) << std::endl;
            continue;
        }

        // Send private message using @username message format
        if (message.find("@") == 0) {
            send(clientSocket, message.c_str(), message.length(), 0);
            std::cout << "[Private] Sent to " << message.substr(1, message.find(" ") - 1) << std::endl;
            continue;
        }

        // Regular chat message
        std::string formattedMessage = username + ": " + message;
        send(clientSocket, formattedMessage.c_str(), formattedMessage.length(), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
