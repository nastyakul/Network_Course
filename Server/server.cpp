#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <list>
#include <fstream>

#include "m_class.h"

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "3504"

const char OPTION_VALUE = 1;

void delete_disconnected_clients(std::list<client_type*> *connected_clients) //указатель на список из подсоединенных клиентов
{
	std::list<client_type*> ::iterator iter = connected_clients->begin();
	while (iter != connected_clients->end())  //итератор, класс, по которому можно итерироваться
	{
		if ((*iter)->socket == INVALID_SOCKET) //разыменование, обращение в сокет
		{
			delete (*iter)->proc;
			delete *iter;
			iter = connected_clients->erase(iter);
			continue;
		}
		iter++;
	}
}

void load_white_list(std::list<std::string>* white_ip)
{
	std::string current_ip;
	std::ifstream white_file;

	white_file.open("C:\\Users\\User\\Desktop\\Networks_Course\\ConsoleApplication2\\white_list.txt", std::ifstream::in);
	while (std::getline(white_file, current_ip))
	{
		white_ip->push_back(current_ip);
	}

	white_file.close();
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
	getaddrinfo("169.254.244.151", DEFAULT_PORT, &hints, &server); //

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

	std::list<std::string> white_ip;

	load_white_list(&white_ip);
	bool ip_is_white = false;


	while (1)
	{
		ip_is_white = false;

		SOCKADDR_IN client_info = { 0 };
		int addrsize = sizeof(client_info);

		SOCKET incoming = INVALID_SOCKET;
		incoming = accept(server_socket, (struct sockaddr*)&client_info, &addrsize); //почитать

																					 // or get it from the socket itself at any time
		addrsize = sizeof(client_info);
		getpeername(incoming, (struct sockaddr*)&client_info, &addrsize);

		char *ip = inet_ntoa(client_info.sin_addr);

		printf("Incomming connection with IP %s\n", ip);

		for (std::list<std::string>::iterator it = white_ip.begin(); it != white_ip.end(); it++)
		{
			if (strcmp(ip, (*it).c_str()) == 0)
			{
				ip_is_white = true;
				break;
			}
		}

		if (!ip_is_white)
		{
			closesocket(incoming);
			printf("Not in white list, refusing...\n");

			continue;
		}

		printf("Connection accepted\n");


		client_type* new_client = new client_type;  //new - это выделение памяти

		new_client->socket = incoming;

		new_client->proc = new m_process();
		new_client->m_threads[READ_PIPE_THREAD] = std::thread(ReadFromPipe, new_client); //0
		new_client->m_threads[WRITE_PIPE_THREAD] = std::thread(WriteToPipe, new_client);  //1
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
