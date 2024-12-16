#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
 
const int BUFFER_SIZE = 1024;
 
struct ClientInfo {
    SOCKET socket;
    string name;
};
 
vector<ClientInfo> clients;
 
void receiveMessages(ClientInfo client) {
    char buffer[BUFFER_SIZE] = { '\0' };
    int retVal;
    while (true) {
        retVal = recv(client.socket, buffer, sizeof(buffer), 0);
        if (retVal == SOCKET_ERROR) {
            cout << "Соединение с клиентом " << client.name << " разорвано" << endl;
            closesocket(client.socket);
 
            // Удалить отключенного клиента из списка
            clients.erase(remove_if(clients.begin(), clients.end(),
                [&](const ClientInfo& c) { return c.socket == client.socket; }), clients.end());
 
            return;
        }
 
        // Отправить сообщение всем клиентам
        for (const auto& otherClient : clients) {
            if (otherClient.socket != client.socket) {
                string message = client.name + ": " + buffer;
                send(otherClient.socket, message.c_str(), message.size(), 0);
            }
        }
 
        cout << client.name << ": " << buffer << endl;
    }
}
 
int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
 
    int retVal = 0;
    WORD ver = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(ver, &wsaData);
 
    SOCKET serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == SOCKET_ERROR) {
        cout << "Не удается создать сокет" << endl;
        WSACleanup();
        return 1;
    }
 
    SOCKADDR_IN serverInfo;
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(2003);
 
    retVal = bind(serverSock, (LPSOCKADDR)&serverInfo, sizeof(serverInfo));
    if (retVal == SOCKET_ERROR) {
        cout << "Не удается связать сокет с адресом" << endl;
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
 
    retVal = listen(serverSock, SOMAXCONN);
    if (retVal == SOCKET_ERROR) {
        cout << "Не удается установить прослушивание" << endl;
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
 
    cout << "Ожидание подключения клиентов..." << endl;
 
    while (true) {
        SOCKADDR_IN clientAddr;
        int addrSize = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &addrSize);
 
        if (clientSock == INVALID_SOCKET) {
            cout << "Ошибка при подключении клиента" << endl;
            closesocket(serverSock);
            WSACleanup();
            return 1;
        }
 
        cout << "Клиент подключен" << endl;
 
        char nameBuffer[BUFFER_SIZE] = { '\0' };
        retVal = recv(clientSock, nameBuffer, sizeof(nameBuffer), 0);
        if (retVal == SOCKET_ERROR) {
            cout << "Ошибка при получении имени клиента" << endl;
            closesocket(clientSock);
            closesocket(serverSock);
            WSACleanup();
            return 1;
        }
 
        string clientName(nameBuffer);
        cout << "Клиент '" << clientName << "' присоединился" << endl;
 
        // Добавить нового клиента в список
        clients.push_back({ clientSock, clientName });
 
        // Запустить поток для обработки сообщений от клиента
        thread(receiveMessages, clients.back()).detach();
    }
 
    closesocket(serverSock);
    WSACleanup();
    return 0;
}