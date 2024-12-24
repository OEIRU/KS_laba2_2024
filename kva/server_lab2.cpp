#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<winsock2.h>
#include<stdio.h>
#include<iostream>
#include<string>
#include<sstream>
#include<vector>
#include<iphlpapi.h>			  // Для работы с интерфейсами
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib") // Дляиспользования IP Helper API
using namespace std;

// Функцияподсчетасимволоввпредложении
int count_chars_in_sentence(const string& sentence) {
    int count = 0;
    for (char c : sentence) {
        if (!isspace(c)) {  // Не учитываем пробелы
            count++;
        }
    }
    return count;
}

// Проверка, является ли IP-адрес локальным (192.168.x.x, 10.x.x.x, 172.16.x.x - 172.31.x.x)
bool is_private_ip(const string& ip) {
    unsigned int b1, b2, b3, b4;
    sscanf(ip.c_str(), "%u.%u.%u.%u", &b1, &b2, &b3, &b4);

    if ((b1 == 10) ||
        (b1 == 172 && b2 >= 16 && b2 <= 31) ||
        (b1 == 192 && b2 == 168)) {
        return true;	 // Этолокальный (приватный) IP
    }
    return false;
}

vector<string> split_by_delimiters(const string& input, const string& delimiters) {
    vector<string> result;
    string current;

    // Перебираем каждый символ в строке
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        // Если символ является разделителем, добавляем текущую строку в результат
        if (delimiters.find(c) != string::npos) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();  // Очистить текущую строку для следующего предложения
            }
            result.push_back(string(1, c));  // Добавляем сам разделитель как отдельный элемент
        }
        else {
            current += c;  // Добавляем символ в текущую строку
        }
    }

    // Добавляем оставшуюся строку, если она не пустая
    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
}

// Функция для получения локального IP-адреса
string get_local_ip() {
    char buffer[128] = { 0 };	// массив для хранения IP-адреса
    // Получаем список всех интерфейсов. Переменная adapterInfo будет указывать на структуру, которая содержит 
 //информацию о сетевых интерфейсах компьютера.
    PIP_ADAPTER_INFO adapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO);

    // Функция GetAdaptersInfo (из Windows API) используется для получения информации о всех сетевых адаптерах //(интерфейсах). Она возвращает ошибку ERROR_BUFFER_OVERFLOW, если выделенного места под одну структуру //недостаточно для хранения всей информации и увеличивает размер буфера (bufferSize), выделяя память заново с
    //правильным размером.
    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
    }

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        //Переменная pAdapterInfo указывает на текущий сетевой адаптер, а цикл while (pAdapterInfo) перебирает все //интерфейсы.
        PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
        while (pAdapterInfo) {
            string ip = pAdapterInfo->IpAddressList.IpAddress.String;//Накаждомшагециклафункцияполучает
            //IP-адрестекущегоадаптера
            if (is_private_ip(ip)) {		// Является ли этот IP-адрес частным (локальным)
                strcpy(buffer, ip.c_str());	// Если найден локальный IP-адрес, он копируется в буфер
                break;		// Цикл прерывается (выход из while), так как нужный IP уже найден.
            }
            pAdapterInfo = pAdapterInfo->Next;
        }
    }

    free(adapterInfo);
    return string(buffer);	//возвращает IP-адрес в виде строки
}


int main(void) {
    WORD sockVer;		// В этой переменной хранится версия Winsock, которую мы хотим использовать.
    WSADATA wsaData;		// Структура WSADATA хранит информацию о реализации сокетов Windows (Winsock)
    int retVal;		// Переменная для отслеживания ошибок
    sockVer = MAKEWORD(2, 2);	// Макрос MAKEWORD(2, 2)создает 16-битное значение, представляющее версию 					Winsock — 2.2
    WSAStartup(sockVer, &wsaData);	// Эта функция инициирует использование Winsock API в программе

    // Создаемсокет
      // Функция создания сокета socket имеет три параметра: 

    SOCKET servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (servSock == INVALID_SOCKET) {
        printf("Unable to create socket\n");
        WSACleanup();
        return SOCKET_ERROR;
    }

    SOCKADDR_IN sin;		// Структура, которая хранит информацию о сетевом адресе, с которым будет 
    //ассоциирован сокет.
    sin.sin_family = PF_INET;
    sin.sin_port = htons(2009);	// Устанавливается порт для работы сервера — 2009.
    sin.sin_addr.s_addr = INADDR_ANY;	// Указывает, что сокет будет прослушивать все сетевые интерфейсы, 
    //доступные на машине

    // Привязка сокета к адресу. bind — это функция, которая связывает сокет с определенным IP-адресом и 
    // портом. В данном случае сокет servSock привязывается к адресу INADDR_ANY, что означает, что сокет будет 
    // принимать подключения на любом доступном интерфейсе).  
    retVal = bind(servSock, (LPSOCKADDR)&sin, sizeof(sin));
    if (retVal == SOCKET_ERROR) {
        printf("Unable to bind\n");
        WSACleanup();
        return SOCKET_ERROR;
    }

    // Получаем и выводим локальный IP-адрес сервера
    string local_ip = get_local_ip();
    if (local_ip.empty()) {
        printf("Unable to determine local IP address\n");
        closesocket(servSock);
        WSACleanup();
        return 1;
    }

    // выводится сообщение с информацией о том, что сервер запущен
    printf("Server started at %s, port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

    while (true) {
        // Ожидание подключения клиента. Сервер теперь готов принимать подключения от клиентов.Второй аргумент 
        // 10 указывает максимальное количество подключений, которые могут ожидать обработки одновременно.
        retVal = listen(servSock, 10);
        if (retVal == SOCKET_ERROR) {
            printf("Unable to listen\n");
            WSACleanup();
            return SOCKET_ERROR;
        }
        // Принятие нового соединения от клиента	
        SOCKET clientSock;
        SOCKADDR_IN from;
        int fromlen = sizeof(from);
        clientSock = accept(servSock, (struct sockaddr*)&from, &fromlen);
        // acceptфункция, которая принимает новое соединение от клиента. Параметры:

        if (clientSock == INVALID_SOCKET) {
            printf("Unable to accept\n");
            WSACleanup();
            return SOCKET_ERROR;
        }
        // Когда клиент подключается, выводится сообщение с IP-адресом клиента и его портом:
        printf("New connection accepted from %s, port %d\n", inet_ntoa(from.sin_addr), htons(from.sin_port));

        // Чтение данных от клиента
        char szReq[256] = { 0 };
        while (true) {
            // recv() — используется для получения данных от клиента через сокет clientSock.
            retVal = recv(clientSock, szReq, 256, 0);
            if (retVal == SOCKET_ERROR) {
                printf("Unable to receive\n");
                closesocket(clientSock);
                break;
            }

            printf("Datareceived: %s\n", szReq);// Если данные были успешно получены, они выводятся на экран
            string input = szReq;

            if (input == "exit") {
                // Закрываемсоединениесданнымклиентом
                printf("Client requested to close connection.\n");
                closesocket(clientSock);
                break;
            }

            // Проверка на наличие разделителей
            if (input.find('.') == string::npos &&
                input.find('?') == string::npos &&
                input.find('!') == string::npos) {
                // Если нет разделителей, не обрабатываем строку
                printf("No periods found, ignoring input.\n");
                string response = "Input must contain at least one period, question mark, or exclamation mark.\n";
                send(clientSock, response.c_str(), response.length(), 0);
                continue;
            }

            // Проверяем, заканчивается ли строка правильным образом
            if (input.back() != '.' && input.back() != '?' && input.back() != '!') {
                size_t lastPeriodPos = input.find_last_of('.');// Еслиточканенайденавконце, программаищет
                //позицию последней точки в строке

                // Если строка не заканчивается одним из разделителей, удаляем последнее предложение
                size_t lastDelimiterPos = input.find_last_of(".!?");
                if (lastDelimiterPos != string::npos) {
                    input = input.substr(0, lastDelimiterPos + 1);  // Оставляемтолькочастьдопоследнегоразделителя
                }
            }

            // Разделение текста по точкам, восклицательным и вопросительным знакам
            vector<string> sentences = split_by_delimiters(input, ".!?");
            string response;

            for (const auto& sentence : sentences) {
                if (!sentence.empty()) {
                    int char_count = count_chars_in_sentence(sentence);
                    response += "Sentence: '" + sentence + "' has " + to_string(char_count) + " characters.\n";
                }
            }
            // Отправка ответа клиенту с помощью функции send(). Еепараметры:
            retVal = send(clientSock, response.c_str(), response.length(), 0);
            if (retVal == SOCKET_ERROR) {
                printf("Unable to send\n");
                closesocket(clientSock);
                break;
            }

            // Очистка буфера для следующего запроса
            memset(szReq, 0, sizeof(szReq));
        }
    }

    // Закрываем серверный сокет только при завершении работы
    closesocket(servSock);
    WSACleanup();		// завершает использование сокетов Windows (Winsock API), освобождая все ресурсы,  связанные с сетевыми операциями.
}