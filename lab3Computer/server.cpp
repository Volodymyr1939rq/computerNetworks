#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

std::vector<SOCKET> clients;

DWORD WINAPI ClientHandler(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char buffer[1024];

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break; 

        buffer[bytesReceived] = '\0';
        std::cout << "[Forum] Accepted: " << buffer << std::endl;
 
        for (SOCKET s : clients) {
            if (s != clientSocket) {
                send(s, buffer, bytesReceived, 0);
            }
        }
    }
    closesocket(clientSocket);
    return 0;
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in tcpAddr;
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_port = htons(9999);
    tcpAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (sockaddr*)&tcpAddr, sizeof(tcpAddr)) == SOCKET_ERROR) {
    
        std::cout << "Error Bind TCP: " << GetLastError() << std::endl; 
        return 1;
    }
    listen(listenSocket, SOMAXCONN);

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in udpAddr;
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port = htons(8888);
    udpAddr.sin_addr.s_addr = INADDR_ANY;
    bind(udpSocket, (sockaddr*)&udpAddr, sizeof(udpAddr));

    std::cout << "Server is ready..." << std::endl;

    CreateThread(NULL, 0, [](LPVOID lp) -> DWORD {
        SOCKET s = (SOCKET)lp;
        char buf[1024];
        sockaddr_in cAddr;
        int cSize = sizeof(cAddr);
        while(true) {
            recvfrom(s, buf, sizeof(buf), 0, (sockaddr*)&cAddr, &cSize);
            sendto(s, "SERVER_FOUND", 12, 0, (sockaddr*)&cAddr, cSize);
        }
        return 0;
    }, (LPVOID)udpSocket, 0, NULL);

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            clients.push_back(clientSocket); 
            CreateThread(NULL, 0, ClientHandler, (LPVOID)clientSocket, 0, NULL);
            std::cout << "New client has been connected to the sever!" << std::endl;
        }
    }

    WSACleanup();
    return 0;
}