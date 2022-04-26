#include <mrudp/mrudp.hpp>

#include <functional>
#include <condition_variable>
#include <chrono>
#include <thread>

#include <iostream>

#include "Echo.h"

struct Server {
	mrudp_service_t service;
	mrudp_addr_t address;
	mrudp_socket_t socket;
	
	std::condition_variable closedEvent;
	bool closed = false;
	
	void start();
	void stop();
} ;

void Server::start()
{
	service = mrudp_service();
	mrudp_addr_t anyAddress;
	mrudp_str_to_addr("127.0.0.1:0", &anyAddress);

	socket = mrudp_socket(service, &anyAddress);
	mrudp_socket_addr(socket, &address);
	
	mrudp_listen(
		socket, nullptr,
		[&](void *, mrudp_connection_t connection) {
			std::cout << "Connection accept" << std::endl;
			
			return mrudp_accept(
				connection,
				nullptr,
				[connection](void *, char *data, int size, int is_reliable) {
					std::cout << "Received [" << std::string(data, size) << "]" << std::endl;
					return mrudp_send(connection, data, size, is_reliable);
				},
				[connection](void *, mrudp_event_t event) {
					std::cout << "Connection closed" << std::endl;
					return mrudp_close_connection(connection);
				}
			);
		},
		[&](void *, mrudp_event_t) {
			std::cout << "Socket closed" << std::endl;
			closedEvent.notify_all();
			return MRUDP_OK;
		}
	);
}

void Server::stop()
{
	mrudp_close_socket(socket);
	mrudp_close_service(service, true);
}

int run_echo_server_cpp(int argc, const char* argv[])
{
	auto *server = new Server();
	server->start();

	const int addressStringSize = 256;
	char addressString[addressStringSize];
	mrudp_addr_to_str(&server->address, addressString, addressStringSize);
	std::cout << "Running server on " << addressString << std::endl;

	int countDown = 10;
	while(--countDown > 0)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	
	server->stop();
	delete server;
	
	return 0;
}
