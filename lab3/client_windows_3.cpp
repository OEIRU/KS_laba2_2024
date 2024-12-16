#pragma comment(lib,"Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <thread>
#include <string>
#include <winsock2.h>
 
using namespace std;
 
const int BUFFER_SIZE = 1024;
 
void receiveMessages(int clientSocket) {
    char buffer[BUFFER_SIZE] = { '\0' };
    int retVal;
    while (true) {
        retVal = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (retVal == SOCKET_ERROR) {
            cout << "Соединение с сервером разорвано" << endl;
            WSACleanup();
            return;
        }
        cout << buffer << endl;
    }
}
 
int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
 
    int retVal = 0;
    WORD ver = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(ver, &wsaData);
 
    SOCKET clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSock == SOCKET_ERROR) {
        cout << "Не удается создать сокет" << endl;
        WSACleanup();
        return 1;
    }
 
    string ip;
    unsigned short port;
 
    cout << "ip: ";
    cin >> ip;
 
    cout << "port: ";
    cin >> port;
 
    cin.ignore(); // очистка потока ввода
 
    SOCKADDR_IN serverInfo;
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    serverInfo.sin_port = htons(port);
 
    retVal = connect(clientSock, (LPSOCKADDR)&serverInfo, sizeof(serverInfo));
    if (retVal == SOCKET_ERROR) {
        cout << "Не удается подключиться" << endl;
        WSACleanup();
        return 1;
    }
 
    cout << "Соединение установлено успешно" << endl;
 
    string name;
    cout << "Введите своё имя: ";
    getline(cin, name);
 
    retVal = send(clientSock, name.c_str(), name.size(), 0);
    if (retVal == SOCKET_ERROR) {
        cout << "Не удается отправить" << endl;
        WSACleanup();
        return 1;
    }
 
    cout << "Можете начинать общение. Для выхода из чата напишите 'stop'" << endl;
 
    thread(receiveMessages, clientSock).detach();
    
    string line;
    while (true) {
        getline(cin, line);
        retVal = send(clientSock, line.c_str(), line.size(), 0);
        if (retVal == SOCKET_ERROR) {
            cout << "Не удается отправить" << endl;
            WSACleanup();
            return 1;
        }
 
        if (line == "stop") {
            break;
        }
    }
 
    closesocket(clientSock);
    WSACleanup();
    return 0;
}