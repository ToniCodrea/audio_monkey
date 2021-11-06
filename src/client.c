#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <winsock2.h>
#include <ws2tcpip.h>

//#include <unistd.h>
#include <io.h>

#include <stdint.h>
#include "server.h"
#include "client.h"
// USING TCP

int client_connect(struct in_addr server_ip, int server_port, SOCKET *client_socket) {
	// Note: make sure to convert a string IP to in_addr_t using "inet_addr( char *IPaddress)";
	*client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket at pointer
	
	struct sockaddr_in server_address;		// Create our target address struct
	server_address.sin_family = AF_INET; // IPV4
	server_address.sin_port = htons(server_port);
	server_address.sin_addr = server_ip;

	int status = connect(*client_socket, (struct sockaddr*) &server_address, sizeof(server_address) );
	return status; // returns -1 on failure
}

int get_server_gamestate(SOCKET *client_socket, char *key_press) {

	int status = recv(*client_socket, key_press, 1, 0);
	return status; // returns -1 on failure
}

int send_server_gamestate(SOCKET *client_socket, char *voice_level) {
	int status = send(*client_socket, voice_level, 1, 0);
	return status;
}

void close_connection(SOCKET *client_socket) {
	_close(*client_socket);
}


/*
int main() {

	gamestate client_state;
	client_state.ypos = 1;
	client_state.xpos = 2;
	client_state.audio = 3;
	int socket;
	printf("Entering\n");
	if (client_connect(INADDR_ANY, 9002, &socket) == -1) {
		perror("Issue connecting");
	}
	printf("Connected\n");
	gamestate new_gamestate;
	if (get_server_gamestate(&socket, &new_gamestate) == -1) {
		perror("Issue getting gamestate");
	}
	printf("Gamestate received\n");
	if (send_server_gamestate(&socket, &client_state) == -1) {
		perror("Issue sending state to server");
	}
	printf("Gamestate sent");

	printf("Server: xpos: %d\nypose: %d\naudio: %d\n", new_gamestate.xpos, new_gamestate.ypos, new_gamestate.audio);
	close_connection(&socket);
}

*/
