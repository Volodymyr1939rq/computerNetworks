#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

DWORD WINAPI ReceiveHandler(LPVOID lpParam) {
    SOCKET sock = (SOCKET)lpParam;
    char buffer[1024];
    while (true) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            std::cout << "\n[System] Connection to the server lost." << std::endl;
            exit(0);
        }
        buffer[bytes] = '\0';
        std::cout << "\nNew message: " << buffer << "\n> " << std::flush;
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    BOOL broadcast = TRUE;
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));

    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(8888);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    std::cout << "[UDP] Searching for a server in the network..." << std::endl;
    sendto(udpSocket, "DISCOVER", 8, 0, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

    sockaddr_in serverAddr;
    int serverAddrSize = sizeof(serverAddr);
    char buf[1024];

    recvfrom(udpSocket, buf, sizeof(buf), 0, (sockaddr*)&serverAddr, &serverAddrSize);
    closesocket(udpSocket);

    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(serverAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    std::cout << "[UDP] Server found at IP address: " << ipStr << std::endl;

    SOCKET tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_port = htons(9999);

    if (connect(tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "TCP connection error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "[TCP] Connected to chat! You can type messages now." << std::endl;

    CreateThread(NULL, 0, ReceiveHandler, (LPVOID)tcpSocket, 0, NULL);

    std::string message;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);
        if (message == "exit") break;
        send(tcpSocket, message.c_str(), message.length(), 0);
    }

    closesocket(tcpSocket);
    WSACleanup();
    return 0;
}