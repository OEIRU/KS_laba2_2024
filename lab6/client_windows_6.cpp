#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h> // Для inet_pton
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int BUFFER_SIZE = 1024;

// Функция для получения сообщений с сервера (для чата)
void receiveMessages(SOCKET sock) {
    char buffer[BUFFER_SIZE] = { 0 };
    while (true) {
        int retVal = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (retVal > 0) {
            buffer[retVal] = '\0'; // Завершаем строку
            cout << buffer << endl;
        }
        else {
            cout << "Connection closed or error occurred" << endl;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return -1;
    }

    cout << "Enter server IP: ";
    string serverIp;
    cin >> serverIp;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return -1;
    }

    SOCKADDR_IN serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(2007);

    // Преобразование IP-адреса в формат IPv4
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid address or address not supported" << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // Подключение к серверу
    if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed" << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    cout << "Choose mode: (1) File Search (2) Chat: ";
    int mode;
    cin >> mode;

    if (mode == 1) {
        send(sock, "FILES", 5, 0);
        cout << "Enter directory path: ";
        string dir;
        cin.ignore(); // Очищаем буфер
        getline(cin, dir);
        send(sock, dir.c_str(), static_cast<int>(dir.size()), 0);

        char buffer[BUFFER_SIZE] = { 0 };
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // Завершаем строку
            cout << buffer << endl;
        }
    }
    else if (mode == 2) {
        send(sock, "CHAT", 4, 0);
        cout << "Enter your name: ";
        string name;
        cin.ignore(); // Очищаем буфер
        getline(cin, name);
        send(sock, name.c_str(), static_cast<int>(name.size()), 0);

        thread(receiveMessages, sock).detach(); // Поток для приёма сообщений
        while (true) {
            string message;
            getline(cin, message);
            send(sock, message.c_str(), static_cast<int>(message.size()), 0);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
