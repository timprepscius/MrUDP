#include <mrudp/mrudp.hpp>

#include <functional>
#include <condition_variable>
#include <chrono>
#include <thread>

#include <iostream>

#include "Echo.h"

struct Client {
	mrudp_service_t service;
	mrudp_addr_t address;
	mrudp_socket_t socket;
	mrudp_connection_t connection;
	
	std::condition_variable closedEvent;
	bool closed = false;
	
	void start(const char *remoteAddressString);
	void stop();
	
	bool send(const char *data, int length);
} ;

void Client::start(const char *remoteAddressString)
{
	service = mrudp_service();
	mrudp_addr_t anyAddress;
	mrudp_str_to_addr("127.0.0.1:0", &anyAddress);
	
	socket = mrudp_socket(service, &anyAddress);
	mrudp_socket_addr(socket, &address);
	
	mrudp_addr_t remoteAddress;
	mrudp_str_to_addr(remoteAddressString, &remoteAddress);

	connection = mrudp_connect(
		socket,
		&remoteAddress,
		nullptr,
		[&](void *_, char *data, int size, int is_reliable) {
			std::cout << "[" << std::string(data, size) << "]" << std::endl;
			return MRUDP_OK;
		},
		[&](void *_, mrudp_event_t reason) {
			std::cout << "conection closed " << reason << std::endl;
			return MRUDP_OK;
		}
	);
}

bool Client::send(const char *data, int length)
{
	return mrudp_send(connection, data, length, 1) == MRUDP_OK;
}

void Client::stop()
{
	mrudp_close_connection(connection);
	mrudp_close_socket(socket);
	mrudp_close_service(service, true);
}

int run_echo_client_cpp(int argc, const char* argv[])
{
	if (argc != 2)
	{
		printf("address not specified\n");
		return -1;
	}

	auto *client = new Client();
	client->start(argv[1]);

	std::cout << "Enter text to send, new line to end." << std::endl;

	std::string line;
	while (1)
	{
		std::getline(std::cin, line);
		
		if (line.empty())
			break;

		if (!client->send(line.data(), (int)line.size()))
		{
			printf("send failed\n");
			return -1;
		}
	}

	client->stop();
	delete client;
	
	return 0;
}
