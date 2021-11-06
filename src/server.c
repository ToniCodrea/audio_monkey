#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>

#include <io.h>

#include <stdint.h>
#include "server.h"


int setup_server(uint32_t server_ip, int server_port, SOCKET *server_socket) {

	*server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// define address to listen to
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server_port);
	server_address.sin_addr.s_addr = server_ip;
	// connect server port to address
	int status = bind(*server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
	//freeaddrinfo(result);
	return status; // -1 on failure
}

int send_data(SOCKET *client_socket, char *key_press) {

	int status = send(*client_socket, key_press, 1, 0);
	return status;

}


void prep_connection(SOCKET *client_socket, SOCKET *server_socket) {

	*client_socket = accept(*server_socket, NULL, NULL);
}

int wait_for_data(SOCKET *client_socket, char *voice_level) {
	int status = recv(*client_socket, voice_level, 1, 0);
	return status;

}


/*
int main() {

	gamestate state_1;
	state_1.ypos = 4;
	state_1.xpos = 5;
	state_1.audio = 6;
	gamestate state_2;

	// need to create some sockets to pass around
	int server_socket;
	int client_socket;
	setup_server(INADDR_ANY, 9002, &server_socket);
	while (1) {
		printf("Server state:\nxpos: %d\nypos: %d\naudio: %d\n",state_1.xpos,state_1.ypos,state_1.audio);
		prep_connection(&client_socket, &server_socket);
		printf("send_data: %d\n",send_data(&client_socket, &state_1));
		printf("data_wait: %d\n",wait_for_data(&client_socket,&state_2));
		printf("Waiting completed\n");
		printf("Client state:\nypos: %d\nxpos: %d\naudio: %d\n",state_2.xpos,state_2.ypos,state_2.audio);
		state_1.xpos++;
		state_1.ypos++;
		state_1.audio++;
	}

	printf("out\n");
	close(server_socket);
	close(client_socket);
}

*/
