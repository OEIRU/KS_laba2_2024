#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>
#include <regex>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int BUFFER_SIZE = 1024;
mutex coutMutex;

void receiveMessages(SOCKET sock) {
    char buffer[BUFFER_SIZE] = { 0 };
    while (true) {
        int retVal = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (retVal > 0) {
            buffer[retVal] = '\0';
            lock_guard<mutex> lock(coutMutex);
            cout << buffer << endl;
        }
        else if (retVal == 0) {
            lock_guard<mutex> lock(coutMutex);
            cout << "[Client] Connection closed by server." << endl;
            break;
        }
        else {
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

    // User selects connection type
    cout << "Choose connection type: (1) Internal (2) External: ";
    int connectionType;
    cin >> connectionType;

    string ip;
    int port;

    if (connectionType == 1) {
        cout << "Enter internal IP address: ";
        cin >> ip;
        port = 2007; // Fixed internal port
    }
    else if (connectionType == 2) {
        cout << "Enter external server IP (format IP:PORT): ";
        string externalIp;
        cin >> externalIp;

        // Parse external IP and port
        regex ipPattern("^(\\d+\\.\\d+\\.\\d+\\.\\d+):(\\d+)$");
        smatch match;
        if (regex_match(externalIp, match, ipPattern)) {
            ip = match[1].str();
            port = stoi(match[2].str());
        }
        else {
            cerr << "[Client] Invalid external IP format. Please use the format IP:PORT." << endl;
            WSACleanup();
            return -1;
        }
    }
    else {
        cerr << "Invalid choice. Exiting." << endl;
        WSACleanup();
        return -1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }

    SOCKADDR_IN serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "[Client] Invalid address or address not supported" << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "[Client] Connection failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    cout << "Choose mode: (1) File Search (2) Chat: ";
    int mode;
    cin >> mode;

    if (mode == 1) { // File Search
        send(sock, "FILES", 5, 0);

        cout << "Enter directory path: ";
        string dir;
        cin.ignore();
        getline(cin, dir);

        send(sock, dir.c_str(), static_cast<int>(dir.size()), 0);

        char buffer[BUFFER_SIZE] = { 0 };
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            cout << "[Client] Received file list:\n" << buffer << endl;
        }
        else if (bytesReceived == 0) {
            cerr << "[Client] Server closed the connection." << endl;
        }
        else {
            cerr << "[Client] Error receiving data from server: " << WSAGetLastError() << endl;
        }

    }
    else if (mode == 2) { // Chat
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

            if (message == "/quit") {
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

    // Cleanup
    shutdown(sock, SD_BOTH);
    closesocket(sock);
    WSACleanup();
    return 0;
}
