#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>

using namespace std;

void list_files_recursive(const string& dir_name, stringstream& output) {
    cout << "Trying to open directory: " << dir_name << endl; // Отладка: вывод пути директории
    DIR* dir = opendir(dir_name.c_str());
    if (!dir) {
        output << "Cannot open directory: " << dir_name << endl;
        perror("opendir");  // Вывод системной ошибки
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string entry_name = entry->d_name;

        // Пропускаем "." и ".."
        if (entry_name == "." || entry_name == "..") continue;

        string full_path = dir_name + "/" + entry_name;
        struct stat entry_info;
        if (stat(full_path.c_str(), &entry_info) != 0) {
            perror(("stat error for " + full_path).c_str());  // Отладка: ошибка вызова stat
            continue;
        }

        if (S_ISDIR(entry_info.st_mode)) {
            // Если это директория, добавляем ее и рекурсивно обрабатываем
            output << "Directory: " << full_path << endl;
            cout << "Recursively entering directory: " << full_path << endl; // Отладка
            list_files_recursive(full_path, output);
        } else {
            // Если это файл, добавляем его
            output << "File: " << full_path << endl;
            cout << "Found file: " << full_path << endl; // Отладка
        }
    }
    closedir(dir);
}

int main() {
    cout << "Starting server..." << endl; // Отладка

    int servSock = socket(AF_INET, SOCK_STREAM, 0);
    if (servSock == -1) {
        perror("Unable to create socket");
        return 1;
    }

    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(2009);
    sin.sin_addr.s_addr = INADDR_ANY;

    if (bind(servSock, (sockaddr*)&sin, sizeof(sin)) == -1) {
        perror("Unable to bind");
        close(servSock);
        return 1;
    }

    if (listen(servSock, 10) == -1) {
        perror("Unable to listen");
        close(servSock);
        return 1;
    }

    cout << "Server started on port 2007" << endl; // Отладка

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int clientSock = accept(servSock, (sockaddr*)&client_addr, &client_len);
        if (clientSock == -1) {
            perror("Unable to accept");
            continue;
        }

        char buffer[256] = {0};
        ssize_t bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            string dir_name(buffer);

            cout << "Received directory request: " << dir_name << endl; // Отладка

            // Обрабатываем запрос и отправляем ответ
            stringstream response;
            list_files_recursive(dir_name, response);
            string response_str = response.str();
            send(clientSock, response_str.c_str(), response_str.size(), 0);
        } else {
            perror("recv");
        }

        close(clientSock);
    }

    close(servSock);
    return 0;
}
