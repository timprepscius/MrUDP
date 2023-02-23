#include <mrudp/mrudp.h>

#include "Platform.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef SYS_WINDOWS
	#include <unistd.h>
#endif

typedef struct {
	mrudp_service_t service;
	mrudp_addr_t address;
	mrudp_socket_t socket;
} server_t;

typedef struct {
	mrudp_service_t service;
	mrudp_socket_t socket;
} client_t;

mrudp_error_code_t server_receive(void *connection_, char *data, int size, int is_reliable)
{
	enum { maxPrintLength = 256 };
	char print[maxPrintLength+1];
	int copySize = size < maxPrintLength ? size : maxPrintLength;
	
	strncpy(print, data, copySize);
	print[copySize] = 0;
	printf("Received [%s]\n", print);
	
	mrudp_connection_t connection = (mrudp_connection_t)connection_;
	return mrudp_send(connection, data, size, is_reliable);
}

mrudp_error_code_t server_connection_close(void *connection_, mrudp_event_t reason)
{
	mrudp_connection_t connection = (mrudp_connection_t)connection_;
	mrudp_close_connection(connection);
	
	printf("Connection closed\n");
	return MRUDP_OK;
}

mrudp_error_code_t server_accept(void *server_, mrudp_connection_t connection)
{
	printf("Connection accept\n");
	return mrudp_accept(connection, connection, server_receive, server_connection_close);
}

mrudp_error_code_t server_close(void *server_, mrudp_event_t reason)
{
	printf("Socket closed\n");
	return MRUDP_OK;
}

server_t *open_server(void)
{
	server_t *server = (server_t *)malloc(sizeof(server_t));
	
	server->service = mrudp_service();
	mrudp_addr_t any_address;
	mrudp_str_to_addr("127.0.0.1:0", &any_address);

	server->socket = mrudp_socket(server->service, &any_address);
	mrudp_socket_addr(server->socket, &server->address);
	
	mrudp_listen(server->socket, server, NULL, server_accept, server_close);
	
	return server;
}

void close_server(server_t *server)
{
	mrudp_close_socket(server->socket);
	mrudp_close_service(server->service, 1);
	
	free(server);
}

int run_echo_server_c(int argc, const char* argv[])
{
	server_t *server = open_server();
	if (!server)
	{
		printf("instantiation of server failed\n");
		abort();
	}
	
	enum { addressStringSize = 256 };
	char addressString[addressStringSize];
	mrudp_addr_to_str(&server->address, addressString, addressStringSize);
	printf("Running server on %s\n", addressString);
	
	int countDown = 60;
	while(countDown-- > 0)
	{
		sleep(1);
	}
	
	printf("closing server\n");
	close_server(server);
	
	return 0;
}
