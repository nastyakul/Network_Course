#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

static const char exit_phrase[] = "Terminate RCP";

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096
#define HOST "localhost"
#define DEFAULT_PORT "3504"

#define EXIT_PROGRAMM(RETURN_VALUE) \
        system("pause");\
        return RETURN_VALUE;

struct client_type
{
        SOCKET socket;
        char received_message[DEFAULT_BUFLEN];
};

int process_client(client_type &new_client)
{
        while (1)
        {
                memset(new_client.received_message, 0, DEFAULT_BUFLEN);

                if (new_client.socket != 0)
                {
                        int iResult = recv(new_client.socket, new_client.received_message, DEFAULT_BUFLEN, 0);

                        if (iResult != SOCKET_ERROR)
                                cout << new_client.received_message;
                        else
                        {
                                cout << "recv() failed: " << WSAGetLastError() << endl;
                                break;
                        }
                }
        }

        if (WSAGetLastError() == WSAECONNRESET)
                cout << "The server has disconnected" << endl;

        return 0;
}

int main()
{
        WSAData wsa_data;
        struct addrinfo *result = NULL, *ptr = NULL, hints;
        client_type client = { INVALID_SOCKET, "" };
        int iResult = 0;
        string message;

        cout << "Starting Client...\n";

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (iResult != 0) {
                cout << "WSAStartup() failed with error: " << iResult << endl;
                EXIT_PROGRAMM(1);
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        cout << "Connecting...\n";

        // Resolve the server address and port
        iResult = getaddrinfo(HOST, DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
                cout << "getaddrinfo() failed with error: " << iResult << endl;
                WSACleanup();
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

        freeaddrinfo(result);

        if (client.socket == INVALID_SOCKET) {
                cout << "Unable to connect to server!" << endl;
                WSACleanup();
                EXIT_PROGRAMM(1);
        }

        cout << "Successfully Connected" << endl;

        bool bSuccess;
        char chBuf[DEFAULT_BUFLEN];
        DWORD dwRead, dwWritten;
        HANDLE hParentStdIn = GetStdHandle(STD_INPUT_HANDLE);

        thread my_thread(process_client, client);

        while (1)
        {
                bSuccess = ReadFile(hParentStdIn, chBuf, DEFAULT_BUFLEN, &dwRead, NULL);

                iResult = send(client.socket, chBuf, dwRead, 0);

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