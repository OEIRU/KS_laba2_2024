#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <winsock2.h>
#include <windows.h>
#include <mutex> // Для работы с mutex и lock_guard
#include <algorithm>
#include <sys/stat.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int SERVER_PORT = 2007;
const int BUFFER_SIZE = 1024;

struct ClientInfo {
    SOCKET socket;
    string name;
};

vector<ClientInfo> chatClients;
mutex chatMutex; // Мьютекс для синхронизации доступа к chatClients

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
        }
        else {
            output << "File: " << full_path << endl;
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
}

// Обработка клиента для поиска файлов
void handleFileClient(SOCKET clientSock) {
    char buffer[BUFFER_SIZE] = {0};
    int bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        string dir_name(buffer);
        cout << "Received directory path: " << dir_name << endl; // Отладочный вывод
        stringstream response;
        list_files_recursive(dir_name, response);
        string response_str = response.str();

        if (response_str.empty()) {
            response_str = "No files found or directory is invalid.";
        }

        cout << "Sending file list to client...\n" << response_str << endl; // Отладочный вывод
        send(clientSock, response_str.c_str(), static_cast<int>(response_str.size()), 0);
    } else {
        string error = "Error reading directory path.";
        send(clientSock, error.c_str(), static_cast<int>(error.size()), 0);
        cout << "Error receiving directory path from client." << endl; // Отладка
    }
    closesocket(clientSock);
}

// Уведомление всех пользователей о событии
void notifyAllClients(const string& message) {
    lock_guard<mutex> lock(chatMutex);
    for (const auto& client : chatClients) {
        send(client.socket, message.c_str(), static_cast<int>(message.size()), 0);
    }
}

// Обработка сообщений в чате
void handleChatClient(ClientInfo client) {
    char buffer[BUFFER_SIZE] = { 0 };
    string welcomeMessage = client.name + " joined the chat\n";
    notifyAllClients(welcomeMessage);
    cout << welcomeMessage;

    while (true) {
        int retVal = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (retVal <= 0) {
            break;
        }
        buffer[retVal] = '\0';
        string message(buffer);

        // Если клиент ввел "exit", выходим из чата
        if (message == "exit") {
            string leaveMessage = client.name + " left the chat\n";
            notifyAllClients(leaveMessage);
            cout << leaveMessage;

            lock_guard<mutex> lock(chatMutex);
            chatClients.erase(remove_if(chatClients.begin(), chatClients.end(),
                [&](const ClientInfo& c) { return c.socket == client.socket; }),
                chatClients.end());
            closesocket(client.socket);
            return;
        }

        string formattedMessage = client.name + ": " + message + "\n";
        notifyAllClients(formattedMessage);
        cout << formattedMessage;
    }

    // Если клиент отключается внезапно
    string disconnectMessage = client.name + " disconnected\n";
    notifyAllClients(disconnectMessage);
    cout << disconnectMessage;

    lock_guard<mutex> lock(chatMutex);
    chatClients.erase(remove_if(chatClients.begin(), chatClients.end(),
        [&](const ClientInfo& c) { return c.socket == client.socket; }),
        chatClients.end());
    closesocket(client.socket);
}

// Обработка клиента: выбор между режимами
void handleClient(SOCKET clientSock) {
    char modeBuffer[BUFFER_SIZE] = { 0 };
    int bytesReceived = recv(clientSock, modeBuffer, sizeof(modeBuffer) - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSock);
        return;
    }
    modeBuffer[bytesReceived] = '\0';
    string mode(modeBuffer);

    if (mode == "FILES") {
        handleFileClient(clientSock);
    }
    else if (mode == "CHAT") {
        char nameBuffer[BUFFER_SIZE] = { 0 };
        bytesReceived = recv(clientSock, nameBuffer, sizeof(nameBuffer) - 1, 0);
        if (bytesReceived > 0) {
            nameBuffer[bytesReceived] = '\0';
            string clientName(nameBuffer);
            {
                lock_guard<mutex> lock(chatMutex);
                chatClients.push_back({ clientSock, clientName });
            }
            thread(handleChatClient, chatClients.back()).detach();
        }
        else {
            closesocket(clientSock);
        }
    }
    else if (mode == "EXIT") {
        string goodbye = "Goodbye!\n";
        send(clientSock, goodbye.c_str(), static_cast<int>(goodbye.size()), 0);
        closesocket(clientSock);
    }
    else {
        string error = "Invalid option. Closing connection.\n";
        send(clientSock, error.c_str(), static_cast<int>(error.size()), 0);
        closesocket(clientSock);
    }
}

// Основной сервер
int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return -1;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return -1;
    }

    SOCKADDR_IN serverInfo = { 0 };
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(SERVER_PORT);
    serverInfo.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (SOCKADDR*)&serverInfo, sizeof(serverInfo)) == SOCKET_ERROR) {
        cerr << "Bind failed" << endl;
        closesocket(serverSock);
        WSACleanup();
        return -1;
    }

    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed" << endl;
        closesocket(serverSock);
        WSACleanup();
        return -1;
    }

    cout << "Server started on port " << SERVER_PORT << endl;

    while (true) {
        SOCKADDR_IN clientAddr;
        int addrSize = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &addrSize);
        if (clientSock != INVALID_SOCKET) {
            thread(handleClient, clientSock).detach();
        }
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
