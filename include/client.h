#ifndef CLIENT_H
#define CLIENT_H

int client_connect(struct in_addr server_ip, int server_port, SOCKET *client_socket);
// Takes in an in_addr_t of the servers ip, which can be acquired as described in the server.h file
// takes in a server port, which must be the same port as the server is set up on
// takes in an int * client_socket which can just be a poiner to an uninitialised int, as this function defines it

int get_server_gamestate(SOCKET *client_socket, char *state_holder);
// takes in the *client_socket as prepared by the client_connect function previous,
// and a gamestate* into which we put the data from the server

int send_server_gamestate(SOCKET *client_socket, char *state_holder);
// takes in the client socket as previously defined, and a gamestate * which contains the info to send
// Much of the same advice about ordering follows from the server.h file
// ensure that client_connect is called before attempting to use get_server_gamestate or send_server_gamestate
// prototype example is in the .c file under a commented function called main

// remember to call close(*client_socket) after finishing the game
void close_connection(SOCKET* client_socket);
#endif
