#ifndef SERVER_H
#define SERVER_H

int setup_server(uint32_t server_ip, int server_port, SOCKET *server_socket);
// Call when starting up the multiplayer session
// Takes in the server's IP address as an in_addr_t, which can be easily supplied by using inet_addr("string of IP address");
// server_port should be an int for a free port, 9002 seemed to work on my machine
// returns -1 on failure

void prep_connection(SOCKET *client_socket, SOCKET *server_socket);
// Takes in a poitner to an int which will represent the active connection which can be uninitialised;
// AND a pointer to another int which should be the same server socket as in the setup_server function
// the first function sets up the server_socket and then we use this function to tell the computer to listen on that socket
// This function will lead to a wait until a connection is attempted (it listens on the client socket port);


int send_data(SOCKET *client_socket, char *state_holder);
// this will send data to the client using a *client_socket, which has to be initialised by the prep_connection function
// you must have called prep_connection before using this
// gamestate just holds the status of the game, and all info you want to synchronise
// you should be able to change this as you want, none of the functions depend on it being anything other than a pointer to a type
// returns -1 on failure

int wait_for_data(SOCKET *client_socket, char *state_holder);
// this takes a gamestate pointer, and will deposit data sent from the client into this struct
// again, must have been initialised by prep_connection, although a wait and a send can both be operated from one prep_connection call

// Important note: ensure the send_data on the server is matched with a receive data on the client
// or vice versa. Having both the client and the server try to send data will lead to a deadlock

// Basic example is in the main file, in the commented function main()

// remember to call close(int *socket) on all sockets opened, namely client_socket and server_socket
// upon finishing the game
#endif
