#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

static const char exit_phrase[] = "Terminate RCP"; //stop connection 

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096
#define HOST "localhost" //127.0.0.1
#define DEFAULT_PORT "3504" //any number, for ex. http port 80

#define EXIT_PROGRAMM(RETURN_VALUE) \
        system("pause");\
        return RETURN_VALUE;

struct client_type //client_connection
{
        SOCKET socket; 
        char received_message[DEFAULT_BUFLEN];
};

int process_client(client_type &new_client) //ссылка на структуру, вот начинается функция!
{
        while (1)
        {
                memset(new_client.received_message, 0, DEFAULT_BUFLEN); //установи столько байтов в буфлене в нули

                if (new_client.socket != 0) //если есть сокет
                {
                        int iResult = recv(new_client.socket, new_client.received_message, DEFAULT_BUFLEN, 0); //откуда, куда записать
                                                                                                               //возвращает число 
                        if (iResult != SOCKET_ERROR)                      //вывод                             //байт
                                cout << new_client.received_message;
                        else
                        {
                                cout << "recv() failed: " << WSAGetLastError() << endl;
                                break;
                        }
                }
        }

        if (WSAGetLastError() == WSAECONNRESET)         //особая ошибочка
                cout << "The server has disconnected" << endl;

        return 0;
}

int main()
{
        WSAData wsa_data;
        struct addrinfo *result = NULL, *ptr = NULL, hints; //два указателя на структуру и структура
        client_type client = { INVALID_SOCKET, "" }; //инициализация
        int iResult = 0;
        string message;

        cout << "Starting Client...\n";

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data); //инициализируем служебное, почитать
        if (iResult != 0) {
                cout << "WSAStartup() failed with error: " << iResult << endl;
                EXIT_PROGRAMM(1);
        }

        ZeroMemory(&hints, sizeof(hints)); //обнуляет память
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
 
        char ip_connect_to[15];
        char port[10];
        int d = 0;
        printf("Enter IP connect to:\n");
        printf("Example: 123.123.123.123\n");
        scanf("%s", ip_connect_to);
       
        printf("Enter PORT connect to:\n");
        printf("Example: 8080\n");
        scanf("%s", port);
       
       
        #define DEFAULT_PORT "3504" //any number, for ex. http port 80
 
        cout << "Connecting...\n";
 
        // Resolve the server address and port
        iResult = getaddrinfo(ip_connect_to, port, &hints, &result); //указ на структуру и на указатель на указатель
        if (iResult != 0) {                     //заполняем исходя из хоста и порта
                cout << "getaddrinfo() failed with error: " << iResult << endl;
                WSACleanup();
                EXIT_PROGRAMM(1);
        }

        // Attempt to connect to an address until one succeeds
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {   //iterate linked list

                // Create a SOCKET for connecting to server
                client.socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); //какой протокол использовать
                if (client.socket == INVALID_SOCKET) {
                        cout << "socket() failed with error: " << WSAGetLastError() << endl;
                        WSACleanup();   //почисть
                        EXIT_PROGRAMM(1);
                }

                // Connect to server.
                iResult = connect(client.socket, ptr->ai_addr, (int)ptr->ai_addrlen); //коннектимся к сокету по адресу
                if (iResult == SOCKET_ERROR) {
                        closesocket(client.socket);
                        client.socket = INVALID_SOCKET;
                        continue;
                }
                break;
        }

        freeaddrinfo(result);

        if (client.socket == INVALID_SOCKET) {
                cout << "Unable to connect to server!" << endl;
                WSACleanup();
                EXIT_PROGRAMM(1);
        }

        cout << "Successfully Connected" << endl;

        bool bSuccess;          //буффер для чтения с клавы
        char chBuf[DEFAULT_BUFLEN]; //для чтения сообщений
        DWORD dwRead, dwWritten; //дабл ворд 4 байта
        HANDLE hParentStdIn = GetStdHandle(STD_INPUT_HANDLE); //для ввода

        thread my_thread(process_client, client);  //потоки

        while (1)
        {
                bSuccess = ReadFile(hParentStdIn, chBuf, DEFAULT_BUFLEN, &dwRead, NULL); //из консоли в чибуф, куда прочитали

                iResult = send(client.socket, chBuf, dwRead, 0); //куда, что, сколько

                if (iResult <= 0)
                {
                        cout << "send() failed: " << WSAGetLastError() << endl;
                        break;
                }

                if (strncmp(chBuf, exit_phrase, strlen(exit_phrase) - 1) == 0) //сравни строчки
                        break;
        }

        //Shutdown the connection since no more data will be sent
        my_thread.detach();

        cout << "Shutting down socket..." << endl;

        closesocket(client.socket);
        WSACleanup();
        EXIT_PROGRAMM(0);
}
