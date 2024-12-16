#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int BUFFER_SIZE = 1024;
mutex coutMutex;

void receiveMessages(SOCKET sock) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        int retVal = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (retVal > 0) {
            buffer[retVal] = '\0';
            lock_guard<mutex> lock(coutMutex);
            cout << buffer << endl;
        } else if (retVal == 0) {
            lock_guard<mutex> lock(coutMutex);
            cout << "[Client] Connection closed by server." << endl;
            break;
        } else {
            lock_guard<mutex> lock(coutMutex);
            cerr << "[Client] Error receiving data: " << WSAGetLastError() << endl;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return -1;
    }

    cout << "Enter server IP: ";
    string serverIp;
    cin >> serverIp;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }

    SOCKADDR_IN serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(2007);

    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid address or address not supported" << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    cout << "Choose mode: (1) File Search (2) Chat: ";
    int mode;
    cin >> mode;

if (mode == 1) { // File Search
    cout << "[DEBUG] File Search mode selected." << endl;
    
    int sendRet = send(sock, "FILES", 5, 0);
    if (sendRet == SOCKET_ERROR) {
        cerr << "[DEBUG] Error sending FILES command: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    cout << "[DEBUG] Sent FILES command to server." << endl;

    cout << "Enter directory path: ";
    string dir;
    cin.ignore();
    getline(cin, dir);
    cout << "[DEBUG] Directory entered by user: " << dir << endl;

    sendRet = send(sock, dir.c_str(), static_cast<int>(dir.size()), 0);
    if (sendRet == SOCKET_ERROR) {
        cerr << "[DEBUG] Error sending directory path: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    cout << "[DEBUG] Sent directory path to server: " << dir << endl;

    char buffer[BUFFER_SIZE] = {0};
    cout << "[DEBUG] Waiting for server response..." << endl;

    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        cout << "[DEBUG] Bytes received from server: " << bytesReceived << endl;
        cout << "[DEBUG] Server response content: " << buffer << endl;
        cout << "[Client] Received file list:\n" << buffer << endl;
    } else if (bytesReceived == 0) {
        cerr << "[DEBUG] Server closed the connection unexpectedly." << endl;
    } else {
        cerr << "[DEBUG] Error receiving data from server: " << WSAGetLastError() << endl;
    }

    } else if (mode == 2) { // Chat
        send(sock, "CHAT", 4, 0);
        cout << "Enter your name: ";
        string name;
        cin.ignore();
        getline(cin, name);
        send(sock, name.c_str(), static_cast<int>(name.size()), 0);

        thread(receiveMessages, sock).detach();
        while (true) {
            string message;
            getline(cin, message);

            if (message == "/quit") { // Graceful exit command
                cout << "[Client] Exiting chat..." << endl;
                break;
            }

            int sendRet = send(sock, message.c_str(), static_cast<int>(message.size()), 0);
            if (sendRet == SOCKET_ERROR) {
                cerr << "[Client] Error sending message: " << WSAGetLastError() << endl;
                break;
            }
        }
    }

    // Graceful cleanup
    shutdown(sock, SD_BOTH);
    closesocket(sock);
    WSACleanup();
    return 0;
}
