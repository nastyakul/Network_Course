#include <winsock2.h>
#include <ws2tcpip.h>                                 //для работы set/get socket
#include <iostream>
#include <string>                                       //memset
#include <thread>

using namespace std;

static const char exit_phrase[] = "Terminate RCP";    //на удаленном компьютере открывается сервер на прослушуку. 
                                                       //Эта фраза прерывает и все осторожно выключает (цикл while 1)

#pragma comment (lib, "Ws2_32.lib")                    //Задает в объектном файле запись поиска библиотеки. Этот тип комментария должен сопровождаться commentstring с именем библиотеки, которую должен найти компоновщик

#define DEFAULT_BUFLEN 4096
#define HOST "localhost"                               //DNS -> 127.0.0.1
#define DEFAULT_PORT "3504"                             // http = 80; 

#define EXIT_PROGRAMM(RETURN_VALUE) \
        system("pause");\
        return RETURN_VALUE;

struct client_type
{
        SOCKET socket;
        char received_message[DEFAULT_BUFLEN];
};

int process_client(client_type &new_client)    //ссылка на структуру
{
        while (1)
        {
                memset(new_client.received_message, 0, DEFAULT_BUFLEN);  //установи DEFAULT_BUFLEN байт там в нули 

                if (new_client.socket != 0)
                {
                        int iResult = recv(new_client.socket, new_client.received_message, DEFAULT_BUFLEN, 0);     //The recv function receives data from a connected socket or a bound connectionless socket.
                                              //откуда, пойнтер на буфер куда, размер буфера, флаг
                        if (iResult != SOCKET_ERROR)
                                cout << new_client.received_message;
                        else
                        {
                                cout << "recv() failed: " << WSAGetLastError() << endl;    //The return value indicates the error code for this thread's last Windows Sockets operation that failed.
                                break;
                        }
                }
        }

        if (WSAGetLastError() == WSAECONNRESET)                             //Connection reset by peer.
                cout << "The server has disconnected" << endl;

        return 0;
}

int main()
{
        WSAData wsa_data;                                                     //Структуры используется для хранения сведения об инициализации Windows Sockets
        struct addrinfo *result = NULL, *ptr = NULL, hints;                   //The addrinfo structure is used by the getaddrinfo function to hold host address information.
        client_type client = { INVALID_SOCKET, "" };
        int iResult = 0;
        string message;

        cout << "Starting Client...\n";

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);              //initiates use of the Winsock DLL by a process.
        if (iResult != 0) {
                cout << "WSAStartup() failed with error: " << iResult << endl;
                EXIT_PROGRAMM(1);
        }

        ZeroMemory(&hints, sizeof(hints));                                    //Обнуление памяти. из Win API а не стандарта C
        hints.ai_family = AF_UNSPEC;                                          //The address family.  У нас unspecified. МБ IPv4, bluetooth и пр
        hints.ai_socktype = SOCK_STREAM;                                      //Provides sequenced, reliable, two-way, connection-based byte streams with an OOB data transmission mechanism. Uses the Transmission Control Protocol (TCP) for the Internet address family
        hints.ai_protocol = IPPROTO_TCP;                                      //

        cout << "Connecting...\n";

        // Resolve the server address and port
        iResult = getaddrinfo(HOST, DEFAULT_PORT, &hints, &result);   //инициализируем стурктуру для доступа к серверу. converts human-readable text strings representing hostnames or IP addresses into a dynamically allocated linked list of struct addrinfo structures. 
        if (iResult != 0) {
                cout << "getaddrinfo() failed with error: " << iResult << endl;
                WSACleanup();                                          //terminates use of the Winsock 2 DLL (Ws2_32.dll).
                EXIT_PROGRAMM(1);
        }

        // Attempt to connect to an address until one succeeds
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

                // Create a SOCKET for connecting to server
                client.socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                if (client.socket == INVALID_SOCKET) {
                        cout << "socket() failed with error: " << WSAGetLastError() << endl;
                        WSACleanup();
                        EXIT_PROGRAMM(1);
                }

                // Connect to server.
                iResult = connect(client.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
                if (iResult == SOCKET_ERROR) {
                        closesocket(client.socket);
                        client.socket = INVALID_SOCKET;
                        continue;
                }
                break;
        }

        freeaddrinfo(result);                                       //frees address information that the getaddrinfo function dynamically allocates in addrinfo structures.

        if (client.socket == INVALID_SOCKET) {
                cout << "Unable to connect to server!" << endl;
                WSACleanup();
                EXIT_PROGRAMM(1);
        }

        cout << "Successfully Connected" << endl;

        bool bSuccess;
        char chBuf[DEFAULT_BUFLEN];
        DWORD dwRead, dwWritten;                                           //каждая переменная по 2*2=4 байта
        HANDLE hParentStdIn = GetStdHandle(STD_INPUT_HANDLE);              // обработчик для stdin; КЛАСС; дескриптор, т.е. число, с помощью которого можно идентифицировать ресурс.

        thread my_thread(process_client, client);                          //запускает в потоке функцию process_client принимающую client

        while (1)
        {
                bSuccess = ReadFile(hParentStdIn, chBuf, DEFAULT_BUFLEN, &dwRead, NULL);   //примерно: stdin открыли как файл

                iResult = send(client.socket, chBuf, dwRead, 0);   //клиент пишет серверу; sends data on a connected socket.

                if (iResult <= 0)
                {
                        cout << "send() failed: " << WSAGetLastError() << endl;
                        break;
                }

                if (strncmp(chBuf, exit_phrase, strlen(exit_phrase) - 1) == 0)
                        break;
        }

        //Shutdown the connection since no more data will be sent
        my_thread.detach();

        cout << "Shutting down socket..." << endl;

        closesocket(client.socket);
        WSACleanup();
        EXIT_PROGRAMM(0);
}
