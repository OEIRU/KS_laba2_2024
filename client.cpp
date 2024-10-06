#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

int main() {
    cout << "Starting client..." << endl; // Отладка

    int clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock == -1) {
        perror("Unable to create socket");
        return 1;
    }

    string ip;
    cout << "Enter server IP: ";
    cin >> ip;

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2007);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    cout << "Connecting to server at " << ip << " on port 2009..." << endl; // Отладка
    if (connect(clientSock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Unable to connect");
        close(clientSock);
        return 1;
    }

    cout << "Connected to server." << endl;

    string dir_name;
    cout << "Enter directory name: ";
    cin >> dir_name;

    cout << "Sending directory request: " << dir_name << endl; // Отладка
    send(clientSock, dir_name.c_str(), dir_name.size(), 0);

    char buffer[4096] = {0};
    ssize_t bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        cout << "Server response:\n" << buffer << endl;
    } else {
        perror("recv");
    }

    close(clientSock);
    cout << "Client disconnected." << endl; // Отладка
    return 0;
}
