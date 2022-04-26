#include <mrudp/mrudp.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	mrudp_service_t service;
	mrudp_socket_t socket;
} client_t;

mrudp_error_code_t client_connection_receive(void *_, char *data, int size, int is_reliable)
{
	printf("Received [%s]\n", data);
	return MRUDP_OK;
}

mrudp_error_code_t client_connection_close(void *_, mrudp_event_t reason)
{
	printf("Connection closed\n");
	return MRUDP_OK;
}

client_t *open_client(void)
{
	client_t *client = (client_t *)malloc(sizeof(client_t));
	
	client->service = mrudp_service();
	mrudp_addr_t any_address;
	mrudp_str_to_addr("127.0.0.1:0", &any_address);

	client->socket = mrudp_socket(client->service, &any_address);
	
	return client;
}

void close_client(client_t *client)
{
	mrudp_close_socket(client->socket);
	mrudp_close_service(client->service, 1);
	
	free(client);
}

int run_echo_client_c(int argc, const char* argv[])
{
	if (argc != 2)
	{
		printf("Address not specified\n");
		return -1;
	}

	client_t *client = open_client();
	if (!client)
	{
		printf("Instantiation of client failed\n");
		return -1;
	}
	
	mrudp_addr_t address;
	mrudp_str_to_addr(argv[1], &address);
	
	mrudp_connection_t connection = mrudp_connect(client->socket, &address, client, client_connection_receive, client_connection_close);
	
	if (!connection)
	{
		printf("Connection failed\n");
		abort();
	}
	
	printf("Enter text to send, new line to end.\n");
	
	char data[256];
	while (1)
	{
		fgets(data, 256, stdin);
		
		int length = (int)strlen(data);
		
		// get rid of the new line
		data[length-1] = 0;

		if (strcmp(data, "") == 0)
			break;

		if (mrudp_failed(mrudp_send(connection, data, length, 1)))
		{
			printf("send failed\n");
			abort();
		}
	}
	
	printf("closing client connection\n");
	mrudp_close_connection(connection);
	sleep(1);
	
	printf("closing client\n");
	close_client(client);
	
	return 0;
}
