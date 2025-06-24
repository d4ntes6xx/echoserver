#pragma warning(disable : 4996)

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <string>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

int read_port_from_config(const char* filename) {
    std::ifstream config(filename);
    std::string line;
    while (std::getline(config, line)) {
        if (line.find("PORT=") == 0) {
            return std::stoi(line.substr(5));
        }
    }
    return 8000;
}

int main(int argc, char** argv) {
    using namespace std;

    const char* configFile = (argc > 1) ? argv[1] : "config.txt";
    int port = read_port_from_config(configFile);

    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        return EXIT_FAILURE;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) != 0)
    {
        fprintf(stderr, "bind() failed: %d\n", WSAGetLastError());
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, SOMAXCONN) != 0)
    {
        fprintf(stderr, "listen() failed: %d\n", WSAGetLastError());
        return EXIT_FAILURE;
    }

    printf("Echo server running on port %d. Waiting for connections...\n", port);

    while (true)
    {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            fprintf(stderr, "accept() failed: %d\n", WSAGetLastError());
            continue;
        }

        printf("Client connected.\n");

        const int BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE];

        while (true) {
            int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived <= 0) {
                printf("Client disconnected or error occurred.\n");
                break;
            }

            buffer[bytesReceived] = '\0';
            printf("Received: %s\n", buffer);

            if (strcmp(buffer, "stop") == 0) {
                printf("Stop command received. Closing connection.\n");
                break;
            }
            else if (strcmp(buffer, "time") == 0) {
                time_t now = time(NULL);
                const char* response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n\r\n"
                    "Time is: ";
                char timeBuffer[BUFFER_SIZE];
                snprintf(timeBuffer, sizeof(timeBuffer), "%s%.24s", response, ctime(&now));
                send(clientSocket, timeBuffer, strlen(timeBuffer), 0);
                printf("Time response sent.\n");
            }
            else {
                send(clientSocket, buffer, bytesReceived, 0);
            }
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();

    return EXIT_SUCCESS;
}
