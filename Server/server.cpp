#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <list>

#include "m_class.h"

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "3504"

const char OPTION_VALUE = 1;

void delete_disconnected_clients(std::list<client_type*> *connected_clients) //указатель на список из подсоединенных клиентов
{
        std::list<client_type*> ::iterator iter = connected_clients -> begin();
        while(iter != connected_clients -> end())  //итератор, класс, по которому можно итерироваться
        {
                if ((*iter) -> socket == INVALID_SOCKET) //разыменование, обращение в сокет
                {
                        delete (*iter)->proc;
                        delete *iter;
                        iter = connected_clients->erase(iter);
                        continue;
                }
                iter++;
        }
}

int main()
{
        WSADATA wsaData;
        struct addrinfo hints;
        struct addrinfo *server = NULL;
        SOCKET server_socket = INVALID_SOCKET;

        std::list<client_type*> connected_clients;

        //Initialize Winsock
        std::cout << "Intializing Winsock..." << std::endl;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        //Setup hints
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        //Setup Server
        std::cout << "Setting up server..." << std::endl;
        getaddrinfo("localhost", DEFAULT_PORT, &hints, &server);

        //Create a listening socket for connecting to server
        std::cout << "Creating server socket..." << std::endl;
        server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

        //Setup socket options
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &OPTION_VALUE, sizeof(int)); 
        setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, &OPTION_VALUE, sizeof(int)); 

                                                                                                       
        std::cout << "Binding socket..." << std::endl;
        bind(server_socket, server->ai_addr, (int)server->ai_addrlen);

        //Listen for incoming connections.
        std::cout << "Listening..." << std::endl;
        listen(server_socket, SOMAXCONN); //читай документацию

        while (1)
        {
                SOCKET incoming = INVALID_SOCKET;
                incoming = accept(server_socket, NULL, NULL); //почитать

                if (incoming == INVALID_SOCKET) continue;

                //Create a temporary id for the next client

                client_type* new_client = new client_type;  //new - это выделение памяти

                new_client -> socket = incoming;

                new_client -> proc = new m_process(); 
                new_client -> m_threads[READ_PIPE_THREAD] = std::thread(ReadFromPipe, new_client); //0
                new_client -> m_threads[WRITE_PIPE_THREAD] = std::thread(WriteToPipe, new_client);  //1
                delete_disconnected_clients(&connected_clients);
                connected_clients.push_back(new_client);
        }


          //Close listening socket
        closesocket(server_socket);

        //Clean up Winsock
        WSACleanup();
        std::cout << "Program has ended successfully" << std::endl;

        system("pause");
        return 0;
}
