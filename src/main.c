#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_timer.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#include "server.h"
#include "client.h"
#include "sound.h"
#include "game.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define FRAMES_PER_SECOND 60

SOCKET client_socket;
SOCKET server_socket;
WSADATA wsaData;
char seed;


void do_connection_thing(int player, char *ip_address) {
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	ip_address++;
	// player = 0 is monke, 1 is voice
	if (player) {
		struct in_addr addr;
		inet_pton(AF_INET, ip_address, &addr);
		while (client_connect(addr, 27015, &client_socket) == -1);
	}
	else {
		setup_server(INADDR_ANY, 27015, &server_socket);
		listen(server_socket, 1);
		do {
			prep_connection(&client_socket, &server_socket);
		} while (client_socket == -1);
	}
}

bool between(int min, int max, int num) {
	return (min <= num) && (num <= max);
}

void add_char(char *text, char ch){
	int index = 0;
	for (; text[index]; index++);
	text[index] = ch;
	text[index + 1] = '\0';
}

void delete_char(char *text){
	int index = 0;
	for (; text[index]; index++);
	text[index - 1] = '\0';
}

int main(int argc, char **argv) {
	// Hide the console
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	// Initialize SDL
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

	if (TTF_Init() < 0) {
		perror("Error loading SDL_ttf.\n");
		exit(EXIT_FAILURE);
	}

	// Create an SDL window and renderer
	SDL_Window *window = SDL_CreateWindow("Sound Bytes", 
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
		WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
	if (!window) {
		perror("Error creating window.\n");
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

  	uint32_t render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, render_flags);
	if (!renderer) {
		perror("Error creating renderer.\n");
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	// Main menu
	SDL_Surface *surface = IMG_Load("../assets/menu_background.png");
	SDL_Texture *menu_background = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect menu_background_rect;
	SDL_QueryTexture(menu_background, NULL, NULL, &menu_background_rect.w, &menu_background_rect.h);
	menu_background_rect = (SDL_Rect) {.x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT};

	// Chosen Box
	surface = IMG_Load("../assets/box.png");
	SDL_Texture *player_box = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect player_box_rect;
	SDL_QueryTexture(player_box, NULL, NULL, &player_box_rect.w, &player_box_rect.h);
	player_box_rect = (SDL_Rect) {.x = 560, .y = 400, .w = 256, .h = 256};
	int player = -1;

	// Voice Calibration Menu
	surface = IMG_Load("../assets/calibrate-mic.png");
	SDL_Texture *calibrate_mic = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	surface = IMG_Load("../assets/calibrate-mic-2.png");
	SDL_Texture *calibrate_mic_2 = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	surface = IMG_Load("../assets/calibrate-mic-3.png");
	SDL_Texture *calibrate_mic_3 = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect calibrate_mic_rect;
	SDL_QueryTexture(calibrate_mic, NULL, NULL, &calibrate_mic_rect.w, &calibrate_mic_rect.h);
	calibrate_mic_rect = (SDL_Rect) {.x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT};
	bool show_mic_menu = false;

	// Text
	TTF_Font *font;
	font = TTF_OpenFont("../assets/BaksoSapi.otf", 52);
	if (!font) {
		perror("Error loading font.\n");
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
	SDL_Surface *text;
	SDL_Color text_color = {57, 59, 51};
	SDL_Texture* text_texture = NULL;

	char ip_text[16] = " ";
	text = TTF_RenderText_Solid(font, ip_text, text_color);
	SDL_Rect ip_box = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 4, text->w, text->h};
	
	// To be used for voice calibration
	int avg_sound_levels = 0;

	SDL_StartTextInput();
	SDL_Event event;
	bool connect = false;
	bool is_recording = false;
	bool render_all = true;
	bool render_text = false;
	bool running = true;

	while (running) {
		render_all = true;
		render_text = false;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_TEXTINPUT:
					// Add characters on text input
					if (strlen(ip_text) < 16) {
						render_text = true;
						add_char(ip_text, event.text.text[0]);
					}
					break;
				case SDL_KEYDOWN:
					// Remove characters with backspace
					if (event.key.keysym.sym == SDLK_BACKSPACE && ip_text[0]) {
						delete_char(ip_text);
						if (!ip_text[0]) {
							ip_text[0] = ' ';
						}
						render_text = true;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					// Check which buttons are pressed
					//printf("x: %d, y: %d\n", event.button.x, event.button.y);
					if (between(420, 640, event.button.y)) {
						if (between(580, 800, event.button.x)) {
							player = 0;
						} else if (between(880, 1100, event.button.x)) {
							player = 1;
							show_mic_menu = true;
						}
					}
					if (show_mic_menu) {
						// Start recording
						if (between(250, 360, event.button.y)) {
							if (between(800, 910, event.button.x)) {
								is_recording = true;
							}
						}
					}
					player_box_rect.x = player ? 860 : 560;
					connect = (player >= 0) && between(100, 560, event.button.x) && between(440, 560, event.button.y);
					break;
				default:
					render_all = false;
					break;
			}		
		}

		// Only render when inputs are detected
		if (render_all) {
			// Clear the window
			SDL_RenderClear(renderer);

			// Draw menu background
			SDL_RenderCopy(renderer, menu_background, NULL, &menu_background_rect);
		
			// Draw text input
			if (render_text) {
				text = TTF_RenderText_Solid(font, ip_text, text_color);
				if (!text) {
					perror("Error loading text.\n");
					SDL_DestroyRenderer(renderer);
					SDL_Quit();
					exit(EXIT_FAILURE);
				}
				text_texture = SDL_CreateTextureFromSurface(renderer, text);
				ip_box.w = text->w; ip_box.h = text->h;
			}
			if (text_texture != NULL) {
				SDL_RenderCopy(renderer, text_texture, NULL, &ip_box);
			}
			// Draw player box
			if (player >= 0){
				SDL_RenderCopy(renderer, player_box, NULL, &player_box_rect);
			}

			// Show voice calibration menu
			if (show_mic_menu) {
				SDL_RenderCopy(renderer, calibrate_mic, NULL, &calibrate_mic_rect);
			}
			if (is_recording) {
				init_mic();
				SDL_RenderCopy(renderer, calibrate_mic_2, NULL, &calibrate_mic_rect);
				SDL_RenderPresent(renderer);
				// Quiet levels
				setup_quiet();

				SDL_RenderCopy(renderer, calibrate_mic_3, NULL, &calibrate_mic_rect);
				SDL_RenderPresent(renderer);
				// Record noise level for a second
				setup_neutral();

				// do some recording stuff with this
				avg_sound_levels = 1000;

				// Have to redraw canvas otherwise a black screen occurs
				SDL_RenderCopy(renderer, menu_background, NULL, &menu_background_rect);
				SDL_RenderCopy(renderer, text_texture, NULL, &ip_box);
				SDL_RenderCopy(renderer, player_box, NULL, &player_box_rect);

				show_mic_menu = false;
				is_recording = false;
			}

			// Present changes made
			SDL_RenderPresent(renderer);
		}
		
		// Run game in 60 fps
		SDL_Delay(1000/FRAMES_PER_SECOND);

		if (connect) {
			do_connection_thing(player, ip_text);
			break;
		}
	}
	SDL_StopTextInput();

	if (!player) {
		char start = 1;
		send_data(&client_socket, &start);
		char client_start;
		wait_for_data(&client_socket, &client_start);
		if (client_start == start) {
			seed = time(NULL);
			send_data(&client_socket, &seed);
			start_game(window, renderer, player, seed);
		}
	} else {
		char server_start;
		get_server_gamestate(&client_socket, &server_start);
		char client_start = 1;
		send_server_gamestate(&client_socket, &client_start);
		if (client_start == server_start) {
			get_server_gamestate(&client_socket, &seed);
			start_game(window, renderer, player, seed);
		}
	}
	// Clean up resources
	close_mic();
	close_connection(&client_socket);
	if (!player) {
		close_connection(&server_socket);
	}
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
