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

// Функция для проверки корректности IP адреса
bool isValidIp(const string& ip) {
    regex ipPattern("^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
        "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
        "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
        "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return regex_match(ip, ipPattern);
}

int main() {
    WORD ver = MAKEWORD(2, 2);
    WSADATA wsaData;
    int retVal = WSAStartup(ver, &wsaData);
    if (retVal != 0) {
        cerr << "WSAStartup failed: " << retVal << endl;
        return 1;
    }

    // Создание сокета
    SOCKET clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSock == INVALID_SOCKET) {
        cerr << "Unable to create socket" << endl;
        WSACleanup();
        return 1;
    }

    // Запрос IP и порта у пользователя
    string serverIp;
    int port;

    cout << "Enter the server IP: ";
    cin >> serverIp;

    // Проверка, что IP корректен
    if (!isValidIp(serverIp)) {
        cerr << "Invalid IP format!" << endl;
        WSACleanup();
        return 1;
    }

    cout << "Enter the server port: ";
    cin >> port;

    // Подключение к серверу
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());
    serverAddr.sin_port = htons(port);

    if (connect(clientSock, (LPSOCKADDR)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Unable to connect to server" << endl;
        closesocket(clientSock);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server at " << serverIp << ":" << port << endl;

    // Ввод имени пользователя
    string name;
    cout << "Enter your name: ";
    cin.ignore();  // Очистка буфера от лишних символов
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
