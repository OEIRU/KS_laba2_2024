#include <sys/types.h>

#include <windows.h>
#include <winsock.h>
#include <sys/stat.h>
#include <sys/types.h>

#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>

#pragma comment(lib, "WS2_32.lib")


using namespace std;
void list_files_recursive(const string& dir_name, stringstream& output) {
    // Отладочный вывод для понимания, какую директорию мы пытаемся открыть
    cout << "Trying to open directory: " << dir_name << endl;

    // Формируем строку для поиска файлов и директорий
    string search_path = dir_name + "\\*"; // Используем обратный слеш для Windows
    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile(wstring(search_path.begin(), search_path.end()).c_str(), &find_data);

    // Если не удалось открыть директорию, выводим ошибку и выходим из функции
    if (hFind == INVALID_HANDLE_VALUE) {
        output << "Cannot open directory: " << dir_name << endl;
        return;
    }

    do {
        wstring ws(find_data.cFileName);
        string entry_name = string(ws.begin(), ws.end());

        // Пропускаем "." и "..", т.к. это текущая и родительская директории
        if (entry_name == "." || entry_name == "..") continue;

        // Формируем полный путь к текущему элементу
        string full_path = dir_name + "\\" + entry_name;

        // Проверяем, является ли элемент директорией
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Если это директория, выводим ее в ответ и рекурсивно обрабатываем ее содержимое
            output << "Directory: " << full_path << endl;
            cout << "Recursively entering directory: " << full_path << endl;  // Отладка
            list_files_recursive(full_path, output);  // Рекурсивный вызов для вложенных директорий
        }
        else {
            // Если это файл, добавляем его в вывод
            output << "File: " << full_path << endl;
            cout << "Found file: " << full_path << endl;  // Отладка
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    // Закрываем дескриптор, когда все элементы обработаны
    FindClose(hFind);
}

int main() {
    WSADATA Data;
    WSAStartup(MAKEWORD(2, 2), &Data);
    cout << "Starting server..." << endl;

    // Создаем сокет. Используем TCP (SOCK_STREAM) и IPv4 (AF_INET)
    int servSock = socket(AF_INET, SOCK_STREAM, 0);
    if (servSock == -1) {
        // Если не удалось создать сокет, выводим ошибку и завершаем программу
        perror("Unable to create socket: ");
        return 1;
    }

    // Структура для описания параметров сокета
    sockaddr_in sin;
    sin.sin_family = AF_INET;  // Используем семейство адресов IPv4
    sin.sin_port = htons(2007);  // Задаем порт 2007, преобразованный в сетевой порядок байтов
    sin.sin_addr.s_addr = INADDR_ANY;  // Сервер будет слушать на всех интерфейсах

    // Привязываем сокет к указанному адресу и порту
    if (bind(servSock, (sockaddr*)&sin, sizeof(sin)) == -1) {
        // Если не удалось привязать сокет, выводим ошибку и завершаем программу
        perror("Unable to bind");
        closesocket(servSock);
        return 1;
    }

    // Начинаем прослушивание входящих соединений (максимум 10 в очереди)
    if (listen(servSock, 10) == -1) {
        // Если не удалось начать прослушивание, выводим ошибку и закрываем сокет
        perror("Unable to listen");
        closesocket(servSock);
        return 1;
    }

    // Сообщаем, что сервер успешно запущен
    cout << "Server started on port 2007" << endl;

    // Основной цикл сервера для обработки входящих соединений
    while (true) {
        sockaddr_in client_addr;
        int client_len = sizeof(client_addr);

        // Принимаем новое входящее соединение от клиента
        int clientSock = accept(servSock, (sockaddr*)&client_addr, &client_len);
        if (clientSock == -1) {
            // Если не удалось принять соединение, выводим ошибку и продолжаем ожидать
            perror("Unable to accept");
            continue;
        }

        // Буфер для хранения сообщения от клиента
        char buffer[256] = { 0 };

        // Получаем данные от клиента (ожидаем имя директории)
        int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            // Завершаем строку, чтобы корректно обработать данные
            buffer[bytesReceived] = '\0';
            string dir_name(buffer);

            // Выводим сообщение о том, какую директорию запросил клиент (отладка)
            cout << "Received directory request: " << dir_name << endl;

            // Формируем ответ: рекурсивно получаем список файлов и поддиректорий
            stringstream response;
            list_files_recursive(dir_name, response);
            string response_str = response.str();

            // Отправляем результат клиенту
            send(clientSock, response_str.c_str(), response_str.size(), 0);
        }
        else {
            // Если произошла ошибка при получении данных от клиента, выводим ошибку
            perror("recv");
        }

        // Закрываем сокет клиента после обработки его запроса
        closesocket(clientSock);
    }

    // Закрываем основной сокет сервера, когда завершается работа (никогда в этом коде)
    closesocket(servSock);
    WSACleanup();
    return 0;
}
    