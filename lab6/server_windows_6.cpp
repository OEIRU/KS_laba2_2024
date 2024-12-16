#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int SERVER_PORT = 2007;
const int BUFFER_SIZE = 1024;

struct ClientInfo {
    SOCKET socket;
    string name;
};

vector<ClientInfo> chatClients;

// Функция для рекурсивного списка файлов
void list_files_recursive(const string& dir_name, stringstream& output) {
    string search_path = dir_name + "\\*";
    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile(wstring(search_path.begin(), search_path.end()).c_str(), &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        output << "Cannot open directory: " << dir_name << endl;
        return;
    }

    do {
        wstring ws(find_data.cFileName);
        string entry_name(ws.begin(), ws.end());
        if (entry_name == "." || entry_name == "..") continue;

        string full_path = dir_name + "\\" + entry_name;
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            output << "Directory: " << full_path << endl;
            list_files_recursive(full_path, output);
        } else {
            output << "File: " << full_path << endl;
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
}

// Обработка клиента для поиска файлов
void handleFileClient(SOCKET clientSock) {
    char buffer[BUFFER_SIZE] = {0};
    int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        string dir_name(buffer);
        stringstream response;
        list_files_recursive(dir_name, response);
        string response_str = response.str();
        send(clientSock, response_str.c_str(), response_str.size(), 0);
    }
    closesocket(clientSock);
}

// Обработка сообщений в чате
void handleChatClient(ClientInfo client) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        int retVal = recv(client.socket, buffer, sizeof(buffer), 0);
        if (retVal <= 0) {
            cout << "Client " << client.name << " disconnected." << endl;
            closesocket(client.socket);
            chatClients.erase(remove_if(chatClients.begin(), chatClients.end(),
                [&](const ClientInfo& c) { return c.socket == client.socket; }), chatClients.end());
            return;
        }
        for (const auto& otherClient : chatClients) {
            if (otherClient.socket != client.socket) {
                string message = client.name + ": " + buffer;
                send(otherClient.socket, message.c_str(), message.size(), 0);
            }
        }
    }
}

// Обработка клиента: выбор между режимами
void handleClient(SOCKET clientSock) {
    char modeBuffer[BUFFER_SIZE] = {0};
    int bytesReceived = recv(clientSock, modeBuffer, sizeof(modeBuffer), 0);
    if (bytesReceived <= 0) {
        closesocket(clientSock);
        return;
    }

    string mode(modeBuffer);
    if (mode == "FILES") {
        handleFileClient(clientSock);
    } else if (mode == "CHAT") {
        char nameBuffer[BUFFER_SIZE] = {0};
        recv(clientSock, nameBuffer, sizeof(nameBuffer), 0);
        string clientName(nameBuffer);
        chatClients.push_back({clientSock, clientName});
        thread(handleChatClient, chatClients.back()).detach();
    } else {
        string error = "Invalid mode.";
        send(clientSock, error.c_str(), error.size(), 0);
        closesocket(clientSock);
    }
}

// Основной сервер
int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN serverInfo = {0};
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(SERVER_PORT);
    serverInfo.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (SOCKADDR*)&serverInfo, sizeof(serverInfo));
    listen(serverSock, SOMAXCONN);
    cout << "Server started on port " << SERVER_PORT << endl;

    while (true) {
        SOCKADDR_IN clientAddr;
        int addrSize = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &addrSize);
        thread(handleClient, clientSock).detach();
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
