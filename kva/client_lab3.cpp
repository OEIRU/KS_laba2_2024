#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <thread>
#include <regex>

using namespace std;

// Функция для приема сообщений от сервера
void receiveMessages(SOCKET clientSock) {
    char buffer[256] = { 0 };
    int retVal;
    while (true) {
        retVal = recv(clientSock, buffer, sizeof(buffer), 0);
        if (retVal == SOCKET_ERROR || retVal == 0) {
            cout << "Disconnected from server." << endl;
            closesocket(clientSock);
            WSACleanup();
            exit(0);
        }
        cout << buffer << endl;  // Печать полученного сообщения
        memset(buffer, 0, sizeof(buffer));  // Очистка буфера
    }
}

int main() {
    WORD ver = MAKEWORD(2, 2);
    WSADATA wsaData;
    int retVal = WSAStartup(ver, &wsaData);
    if (retVal != 0) {
        cerr << "WSAStartup failed: " << retVal << endl;
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
    if (clientSock == INVALID_SOCKET) {
        cerr << "Unable to create socket" << endl;
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "[Client] Invalid address or address not supported" << endl;
        closesocket(clientSock);
        WSACleanup();
        return 1;
    }

    // Подключение к серверу
    if (connect(clientSock, (LPSOCKADDR)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Unable to connect to server" << endl;
        closesocket(clientSock);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server at " << ip << ":" << port << endl;

    // Ввод имени пользователя
    string name;
    cout << "Enter your name: ";
    cin.ignore();
    getline(cin, name);

    retVal = send(clientSock, name.c_str(), name.length(), 0);
    if (retVal == SOCKET_ERROR) {
        cerr << "Unable to send name to server" << endl;
        closesocket(clientSock);
        WSACleanup();
        return 1;
    }

    // Запуск потока для приема сообщений от сервера
    thread(receiveMessages, clientSock).detach();

    // Основной цикл отправки сообщений на сервер
    string input;
    while (true) {
        getline(cin, input);
        if (input == "exit") {
            cout << "Disconnecting..." << endl;
            break;
        }

        if (input.length() > 255) {
            cout << "Message too long. Please limit to 255 characters." << endl;
            continue;
        }

        retVal = send(clientSock, input.c_str(), input.length(), 0);
        if (retVal == SOCKET_ERROR) {
            cerr << "Failed to send message" << endl;
            break;
        }
    }

    // Завершение работы
    closesocket(clientSock);
    WSACleanup();
    return 0;
}
