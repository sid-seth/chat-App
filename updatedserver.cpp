#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <set>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

std::unordered_map<std::string, SOCKET> activeClients;  // Online users
std::unordered_map<std::string, std::vector<std::string>> offlineMessages;  // Buffer for offline users
std::unordered_map<std::string, std::set<std::string>> groupMembers;  // Store group memberships
std::mutex clientsMutex;

std::unordered_map<std::string, std::string> userDatabase = { 
    {"user1", "password123"}, 
    {"user2", "securepass"}, 
    {"user3", "mypassword"} 
};

bool authenticateUser(const std::string& credentials, std::string& username) {
    size_t delimiterPos = credentials.find(':');
    if (delimiterPos == std::string::npos) return false;
    
    username = credentials.substr(0, delimiterPos);
    std::string password = credentials.substr(delimiterPos + 1);

    return userDatabase.count(username) && userDatabase[username] == password;
}

void handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE);
    recv(clientSocket, buffer, BUFFER_SIZE, 0);
    
    std::string credentials(buffer);
    std::string username;
    
    if (!authenticateUser(credentials, username)) {
        std::string authFailed = "Authentication Failed";
        send(clientSocket, authFailed.c_str(), authFailed.length(), 0);
        closesocket(clientSocket);
        return;
    }

    std::string welcomeMessage = "Welcome " + username + "! You can now chat.";
    send(clientSocket, welcomeMessage.c_str(), welcomeMessage.length(), 0);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        activeClients[username] = clientSocket;
    }

    std::cout << username << " joined the chat.\n";

    // Deliver offline messages
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        if (offlineMessages.count(username)) {
            for (const auto& msg : offlineMessages[username]) {
                send(clientSocket, msg.c_str(), msg.length(), 0);
            }
            offlineMessages.erase(username);
        }
    }

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;

        std::string message(buffer);

        // Handle group creation
        if (message.find("/create ") == 0) {
            std::string groupName = message.substr(8);
            std::lock_guard<std::mutex> lock(clientsMutex);
            groupMembers[groupName].insert(username);
            send(clientSocket, ("Group " + groupName + " created!").c_str(), groupName.length() + 10, 0);
            continue;
        }

        // Handle group joining
        if (message.find("/join ") == 0) {
            std::string groupName = message.substr(6);
            std::lock_guard<std::mutex> lock(clientsMutex);
            groupMembers[groupName].insert(username);
            send(clientSocket, ("Joined group " + groupName + "!").c_str(), groupName.length() + 10, 0);
            continue;
        }

        // Handle group messaging
        if (message.find("#") == 0) {
            size_t spacePos = message.find(" ");
            if (spacePos != std::string::npos) {
                std::string groupName = message.substr(1, spacePos - 1);
                std::string groupMsg = "[Group " + groupName + "] " + username + ": " + message.substr(spacePos + 1);

                std::lock_guard<std::mutex> lock(clientsMutex);
                if (groupMembers.count(groupName)) {
                    for (const auto& member : groupMembers[groupName]) {
                        if (activeClients.count(member)) {
                            send(activeClients[member], groupMsg.c_str(), groupMsg.length(), 0);
                        }
                    }
                } else {
                    send(clientSocket, "Group does not exist!", 20, 0);
                }
                continue;
            }
        }

        // Handle private messages
        if (message.find("@") == 0) {
            size_t spacePos = message.find(" ");
            if (spacePos != std::string::npos) {
                std::string recipient = message.substr(1, spacePos - 1);
                std::string privateMsg = "[Private] " + username + ": " + message.substr(spacePos + 1);

                std::lock_guard<std::mutex> lock(clientsMutex);
                if (activeClients.count(recipient)) {
                    send(activeClients[recipient], privateMsg.c_str(), privateMsg.length(), 0);
                } else {
                    offlineMessages[recipient].push_back(privateMsg);
                }
                continue;
            }
        }

        std::string formattedMessage = "[" + username + "]: " + message;
        std::cout << formattedMessage << std::endl;

        std::lock_guard<std::mutex> lock(clientsMutex);
        for (const auto& client : activeClients) {
            if (client.first != username) {
                send(client.second, formattedMessage.c_str(), formattedMessage.length(), 0);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        activeClients.erase(username);
    }

    std::cout << username << " left the chat.\n";
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 10);

    std::cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
