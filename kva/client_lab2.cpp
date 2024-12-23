#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <winsock2.h>
#include <string>
#include <regex>
#include <iostream>
using namespace std;

int main() {
    WORD ver = MAKEWORD(2, 2);
    WSADATA wsaData;
    int retVal = 0;
    if (WSAStartup(ver, (LPWSADATA)&wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Выбор типа подключения
    cout << "Choose connection type: (1) Internal (2) External: ";
    int connectionType;
    cin >> connectionType;

    string ip;
    int port;

    if (connectionType == 1) {
        cout << "Enter internal IP address: ";
        cin >> ip;
        port = 2007; // Фиксированный порт для внутреннего подключения
    } else if (connectionType == 2) {
        cout << "Enter external server IP (format IP:PORT): ";
        string externalIp;
        cin >> externalIp;

        // Парсинг внешнего IP и порта
        regex ipPattern("^(\\d+\\.\\d+\\.\\d+\\.\\d+):(\\d+)$");
        smatch match;
        if (regex_match(externalIp, match, ipPattern)) {
            ip = match[1].str();
            port = stoi(match[2].str());
        } else {
            cerr << "[Client] Invalid external IP format. Please use the format IP:PORT." << endl;
            WSACleanup();
            return -1;
        }
    } else {
        cerr << "Invalid choice. Exiting." << endl;
        WSACleanup();
        return -1;
    }

    // Создание сокета
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSock == SOCKET_ERROR) {
        printf("Unable to create socket\n");
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера
    SOCKADDR_IN serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverInfo.sin_addr) <= 0) {
        cerr << "[Client] Invalid address or address not supported" << endl;
        closesocket(clientSock);
        WSACleanup();
        return 1;
    }

    // Присоединение к серверу
    retVal = connect(clientSock, (LPSOCKADDR)&serverInfo, sizeof(serverInfo));
    if (retVal == SOCKET_ERROR) {
        printf("Unable to connect\n");
        closesocket(clientSock);
        WSACleanup();
        return 1;
    }

    printf("Connection made successfully\n");

    string input;
    while (true) {
        // Ввод строки текста от пользователя
        printf("Enter a sentence (or 'exit' to quit): ");
        cin.ignore();
        getline(cin, input);

        if (input == "exit") {
            // Отправляем на сервер команду выхода, но не закрываем сразу клиент
            retVal = send(clientSock, input.c_str(), input.length(), 0);
            if (retVal == SOCKET_ERROR) {
                printf("Unable to send\n");
                WSACleanup();
                return 1;
            }

            printf("Closing client connection...\n");
            break;  // Выходим из цикла, завершаем клиент
        }

        // Проверка на слишком длинную строку
        if (input.length() > 255) {
            cout << "Input is too long. Please enter a shorter sentence." << endl;
            continue;
        }

        // Отправка строки на сервер
        retVal = send(clientSock, input.c_str(), input.length(), 0);
        if (retVal == SOCKET_ERROR) {
            printf("Unable to send\n");
            WSACleanup();
            return 1;
        }

        // Ожидание ответа от сервера
        char szResponse[256] = { 0 };
        retVal = recv(clientSock, szResponse, sizeof(szResponse), 0);
        if (retVal == SOCKET_ERROR) {
            printf("Unable to receive\n");
            WSACleanup();
            return 1;
        }

        // Печать ответа от сервера
        printf("Server response: %s\n", szResponse);
    }

    closesocket(clientSock);
    WSACleanup();
    return 0;
}
