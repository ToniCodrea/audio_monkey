#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>
#include <math.h>
#include <io.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_timer.h>
#include <SDL_ttf.h>
#include "game.h"
#include "sound.h"
#include "server.h"
#include "client.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define JUMP_SPEED 20
#define TERMINAL_SPEED 10
#define GRAVITY 1
#define FRAMES_PER_SECOND 60
#define MOVEMENT_SPEED 3
#define FALLING_SPEED 4
#define BACKGROUND_SPEED 3
#define NUM_BIRDS 3
#define BIRD_SPEED 6
#define BIRD_WIDTH 64
#define NUM_PLATFORMS 6
#define PLATFORM_SIZE (64*2)
#define PLATFORM_MAX_HEIGHT 360
#define PLATFORM_GAP ((WINDOW_WIDTH - (NUM_PLATFORMS - 1) * PLATFORM_SIZE) / NUM_PLATFORMS)
#define MAX_WAVE 6.0

extern SOCKET client_socket;
extern SOCKET server_socket;
int total_assets;

typedef struct {
	SDL_Texture* texture;
	SDL_Rect* src_rect;
	SDL_Rect* texture_rect;
} object_t;

typedef struct {
	object_t objects[16];
} assets_t;

void game_over_screen(SDL_Window* window, SDL_Renderer* renderer, char* score, assets_t* assets, int player) {
	// Play again window pop-up
	SDL_Surface* surface = IMG_Load("../assets/play-again.png");
	SDL_Texture* play_again = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect play_again_rect;
	SDL_QueryTexture(play_again, NULL, NULL, &play_again_rect.w, &play_again_rect.h);
	play_again_rect = (SDL_Rect){ .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };

	// Chosen Box
	surface = IMG_Load("../assets/box.png");
	SDL_Texture* player_box = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect player_box_rect;
	SDL_QueryTexture(player_box, NULL, NULL, &player_box_rect.w, &player_box_rect.h);
	player_box_rect = (SDL_Rect){ .x = 450, .y = 380, .w = 128, .h = 128 };
	char again = -1;

	// Score display
	TTF_Font* font;
	font = TTF_OpenFont("../assets/BaksoSapi.otf", 52);
	if (!font) {
		perror("Error loading font.\n");
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
	SDL_Surface* text;
	SDL_Color text_color = { 57, 59, 51 };
	text = TTF_RenderText_Solid(font, score + 6, text_color);
	SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text);
	SDL_Rect score_rect = { WINDOW_WIDTH / 2, 230, text->w, text->h };

	SDL_Event event;
	bool render_all = true;
	printf("Player %d\n", player);
	printf("again %d\n", again);
	int running = 1;
	while (running) {
		if (render_all) {
			// Clear window
			SDL_RenderClear(renderer);
			// Render background for consistency
			for (int i = 0; i < total_assets; i++) {
				SDL_RenderCopy(renderer, assets->objects[i].texture, assets->objects[i].src_rect, assets->objects[i].texture_rect);
			}
			// Draw play again menu
			SDL_RenderCopy(renderer, play_again, NULL, &play_again_rect);

			// Draw score
			SDL_RenderCopy(renderer, text_texture, NULL, &score_rect);

			// Draw player box
			if (again >= 0) {
				SDL_RenderCopy(renderer, player_box, NULL, &player_box_rect);
			}
			SDL_RenderPresent(renderer);
		}
		while (again == -1) {
			SDL_PollEvent(&event);
			switch (event.type) {
			case SDL_QUIT:
				return;
				break;
			case SDL_MOUSEBUTTONDOWN:
				// Check which buttons are pressed
				if (between(420, 460, event.button.y)) {
					if (between(450, 580, event.button.x)) {
						again = 1;
					}
					else if (between(720, 820, event.button.x)) {
						again = 0;
					}
					printf("again %d\n", again);
				}
				player_box_rect.x = again ? 450 : 720;
				break;
			default:
				break;
			}
		}
		running = 0;
		char other_choice;
		if (!player) {
			send_data(&client_socket, &again);
			wait_for_data(&client_socket, &other_choice);
			if (!other_choice || !again) {
				break;
			}
			else {
				char seed = time(NULL);
				send_data(&client_socket, &seed);
				start_game(window, renderer, player, seed);
			}
		}
		else {
			get_server_gamestate(&client_socket, &other_choice);
			send_server_gamestate(&client_socket, &again);
			if (!other_choice || !again) {
				break;
			}
			else {
				char seed;
				get_server_gamestate(&client_socket, &seed);
				start_game(window, renderer, player, seed);
			}
			// if player of other player is 1 (yes), then call start_game
			// if player of other player is 0, then call break
			// if player of other player is -1, then keep looping and wait
		}
	}
}

void game_over(SDL_Window* window, SDL_Renderer* renderer, assets_t* assets, int player_index, char* score, int player) {
	float y_vel = 0;
	while (assets->objects[player_index].texture_rect->y <= WINDOW_HEIGHT) {
		y_vel += GRAVITY;
		assets->objects[player_index].texture_rect->y += (int)y_vel;

		// Clear the window
		SDL_RenderClear(renderer);
		// Draw images to window
		for (int i = 0; i < total_assets; i++) {
			SDL_RenderCopy(renderer, assets->objects[i].texture, assets->objects[i].src_rect, assets->objects[i].texture_rect);
		}
		// Show new frame
		SDL_RenderPresent(renderer);
		SDL_Delay(1000 / FRAMES_PER_SECOND);
	}
	game_over_screen(window, renderer, score, assets, player);
}

void oscillate_flood(SDL_Rect* flood_rect, int time, float wave_scale) {
	int to_add = -(int)(wave_scale * cos(time * 3.14 / 180));
	flood_rect->y += to_add;
}

void grow_the_platform(SDL_Rect* platform, unsigned char voice_level) {

	// if (platform->y < WINDOW_HEIGHT - 128) {
	// 	platform->y += FALLING_SPEED;
	// }
	float offset = voice_level / 30.0;
	if (offset > FALLING_SPEED) {
		platform->y -= offset;
	}
	if (platform->y < PLATFORM_MAX_HEIGHT) {
		platform->y = PLATFORM_MAX_HEIGHT;
	}
}

void regulate_player_movement(SDL_Rect* player_rect, float* y_vel, bool up, bool* is_on_the_ground, int current_platform_height) {
	// Calculate vertical velocity
	if (up) {
		if (*is_on_the_ground) {
			*y_vel = -JUMP_SPEED;
			*is_on_the_ground = false;
		}
		// Easing
		// should be less than gravity so no infinite jump
		*y_vel -= 0.80;
	}
	*y_vel += GRAVITY;
	if (fabsf(*y_vel) > TERMINAL_SPEED) {
		*y_vel = (fabsf(*y_vel) / *y_vel) * TERMINAL_SPEED;
	}
	// Move vertical position
	player_rect->y += *y_vel;

	// Collision detection with bounds
	if (player_rect->y <= 0) {
		player_rect->y = 0;
	}
	if (player_rect->y >= current_platform_height - player_rect->h) {
		*is_on_the_ground = true;
		player_rect->y = current_platform_height - player_rect->h;
		*y_vel = 0;
	}
	else {
		*is_on_the_ground = false;
	}
}

void start_game(SDL_Window* window, SDL_Renderer* renderer, int player, char seed) {
	srand(seed);
	float speed = 1.0;
	int wave_size = 2.0;
	uint32_t score = 0;
	assets_t* assets = (assets_t*)malloc(sizeof(assets_t));
	total_assets = 0;
	if (assets == NULL) {
		perror("Couldn't allocate memory for assets\n");
		exit(EXIT_FAILURE);
	}
	// Background Initialization
	SDL_Surface* surface = IMG_Load("../assets/background.png");
	SDL_Texture* background = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect backgrounds[2];
	total_assets += 2;
	for (int i = 0; i < total_assets; i++) {
		SDL_QueryTexture(background, NULL, NULL, &backgrounds[i].w, &backgrounds[i].h);
		backgrounds[i] = (SDL_Rect){ .x = 2 * i * WINDOW_WIDTH, .y = 0, .w = 2 * WINDOW_WIDTH, .h = WINDOW_HEIGHT };
		assets->objects[i] = (object_t){ .texture = background, .texture_rect = backgrounds + i };
	}
	int leading_background = 0;
	// birds Initialization
	surface = IMG_Load("../assets/bird-sheet.png");
	SDL_Texture* bird = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect birds[NUM_BIRDS];
	SDL_Rect bird_srcrect = { 0, 0, 32, 16 };
	for (int i = 0; i < NUM_BIRDS; i++) {
		SDL_QueryTexture(bird, NULL, NULL, &birds[i].w, &birds[i].h);
		birds[i].x = WINDOW_WIDTH + 32 + i * 300;
		birds[i].y = 100 + 10 * (rand() % 40);
		birds[i].w = 64; birds[i].h = 32;
		assets->objects[total_assets + i] = (object_t){ .texture = bird, .src_rect = &bird_srcrect, .texture_rect = &birds[i] };
	}
	total_assets += NUM_BIRDS;

	int next_bird = 0;
	int leading_bird = 0;
	bool bird_is_in_player_region = false;

	// Player Initialization
	surface = IMG_Load("../assets/player-sheet.png");

	if (!surface) {
		perror("Error creating surface.\n");
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	SDL_Texture* player_texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect player_rect;
	SDL_QueryTexture(player_texture, NULL, NULL, &player_rect.w, &player_rect.h);
	player_rect.w = 2 * 32; player_rect.h = 2 * 24;
	SDL_Rect player_srcrect = { 0, 0, 32, 24 };
	assets->objects[total_assets] = (object_t){ .texture = player_texture, .src_rect = &player_srcrect, .texture_rect = &player_rect };
	int player_index = total_assets;
	total_assets += 1;
	surface = IMG_Load("../assets/player_jump.png");
	SDL_Texture* player_jump = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);

	// Player position data
	float y_vel = 0;
	float y_pos = (WINDOW_HEIGHT - player_rect.h) / 2;
	player_rect.x = (int)(WINDOW_WIDTH - player_rect.w) / 4;
	player_rect.y = (int)y_pos;

	bool is_on_the_ground = false;
	SDL_Rect player_bounds_rect = { .x = player_rect.x, .y = 0, .w = player_rect.w, .h = WINDOW_HEIGHT };

	// Platform Initialization
	SDL_Texture* platform_textures[3];
	surface = IMG_Load("../assets/platform_1.png");
	platform_textures[0] = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	surface = IMG_Load("../assets/platform_2.png");
	platform_textures[1] = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	surface = IMG_Load("../assets/platform_3.png");
	platform_textures[2] = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);

	SDL_Rect platforms[NUM_PLATFORMS];
	for (int i = 0; i < NUM_PLATFORMS; i++) {
		platforms[i].x = WINDOW_WIDTH / 4 + i * (PLATFORM_GAP + PLATFORM_SIZE);
		platforms[i].y = WINDOW_HEIGHT - 140 - (rand() % 50);
		SDL_QueryTexture(platform_textures[i % 3], NULL, NULL, &platforms[i].w, &platforms[i].h);
		platforms[i].w *= 2; platforms[i].h *= 2;
		assets->objects[total_assets + i] = (object_t){ .texture = platform_textures[i % 3], .texture_rect = platforms + i };
	}
	total_assets += NUM_PLATFORMS;

	// Flood Initialization
	surface = IMG_Load("../assets/flood-sheet.png");
	SDL_Texture* flood = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect flood_rect;
	flood_rect.x = 0; flood_rect.y = WINDOW_HEIGHT;
	SDL_QueryTexture(flood, NULL, NULL, &flood_rect.w, &flood_rect.h);
	flood_rect.w = WINDOW_WIDTH; flood_rect.h = WINDOW_HEIGHT;
	SDL_Rect flood_srcrect = { 0, 0, 320, 180 };

	assets->objects[total_assets] = (object_t){ .texture = flood, .src_rect = &flood_srcrect, .texture_rect = &flood_rect };
	total_assets += 1;

	// Platform variables
	int next_platform = 0;
	int leading_platform = 0;
	bool platform_is_in_player_region = false;
	int current_platform_height = WINDOW_HEIGHT + player_rect.y;
	SDL_Rect intersection;

	uint32_t time = 0;
	bool running = true;
	bool up = false;
	SDL_Event event;

	// Score UI initialization
	TTF_Font* font;
	font = TTF_OpenFont("../assets/BaksoSapi.otf", 52);
	if (!font) {
		perror("Error loading font.\n");
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
	SDL_Surface* text;
	SDL_Color text_color = { 57, 59, 51 };
	SDL_Texture* text_texture;
	char* score_text = "Score: ";
	char* score_nr = malloc(sizeof(int) * 8 + 1);
	assert(score_nr);
	sprintf(score_nr, "%d", score);
	char* full_score_text = malloc(strlen(score_nr) + strlen(score_text) + 1);
	assert(full_score_text);
	strcpy(full_score_text, score_text);
	strcat(full_score_text, score_nr);
	text = TTF_RenderText_Solid(font, full_score_text, text_color);
	text_texture = SDL_CreateTextureFromSurface(renderer, text);
	SDL_Rect text_rect = { 0, 0, text->w, text->h };

	int player_rate = 0;
	int send = 0;
	while (running) {
		while (SDL_PollEvent(&event)) {
			if (!player) {
				switch (event.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.scancode) {
					case SDL_SCANCODE_W:
					case SDL_SCANCODE_UP:
						up = true;
						break;
					default:
						break;
					}
					break;
				case SDL_KEYUP:
					switch (event.key.keysym.scancode) {
					case SDL_SCANCODE_W:
					case SDL_SCANCODE_UP:
						up = false;
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
			}
		}
		if (send % 4 == 0) {
			if (!player) {
				send_data(&client_socket, &up);
			}
			else {
				get_server_gamestate(&client_socket, &up);
			}
			// Grow platform
			char voice_level;
			if (player) {
				voice_level = get_level();
				send_server_gamestate(&client_socket, &voice_level);
			}
			else {
				wait_for_data(&client_socket, &voice_level);
			}
			grow_the_platform(platforms + (next_platform + 1) % NUM_PLATFORMS, voice_level);
			send = 3;
		}
		// Check for collision with next platform
		if (SDL_IntersectRect(platforms + next_platform, &player_bounds_rect, &intersection) == SDL_TRUE) {
			platform_is_in_player_region = true;
			current_platform_height = platforms[next_platform].y;
			if (platforms[next_platform].y < player_rect.y + player_rect.h - 20) {
				// not sure about this?
				game_over(window, renderer, assets, player_index, full_score_text, player);
				break;
			}
		}
		else {
			current_platform_height = WINDOW_HEIGHT + player_rect.y;
			if (platform_is_in_player_region) {
				next_platform = (next_platform + 1) % NUM_PLATFORMS;
				score++;
				if (score % 10 == 0) {
					speed += 0.1;
					wave_size += 0.3;
					wave_size = min(wave_size, MAX_WAVE);
				}
				platform_is_in_player_region = false;
			}
		}
		// Check for collision with next bird
		if (SDL_IntersectRect(birds + next_bird, &player_bounds_rect, &intersection) == SDL_TRUE) {
			bird_is_in_player_region = true;
			if (SDL_IntersectRect(birds + next_bird, &player_rect, &intersection) == SDL_TRUE) {
				game_over(window, renderer, assets, player_index, full_score_text, player);
				break;
			}
		}
		else {
			if (bird_is_in_player_region) {
				next_bird = 1 - next_bird;
				bird_is_in_player_region = false;
			}
		}
		// Check when to loop background
		if (backgrounds[leading_background].x <= -2 * WINDOW_WIDTH) {
			backgrounds[leading_background].x = backgrounds[1 - leading_background].x + 2 * WINDOW_WIDTH;
			leading_background = 1 - leading_background;
		}
		// Check when to loop bird
		if (birds[leading_bird].x <= -BIRD_WIDTH) {
			birds[leading_bird].x = WINDOW_WIDTH + 200;
			birds[leading_bird].y = 100 + 10 * (rand() % 40);
			leading_bird = (1 + leading_bird) % NUM_BIRDS;
		}

		// Move the player
		regulate_player_movement(&player_rect, &y_vel, up, &is_on_the_ground, current_platform_height);

		// Move platforms
		for (int i = 0; i < NUM_PLATFORMS; i++) {
			// if player passes a platform it should just fall
			if (platforms[i].x + PLATFORM_SIZE < player_rect.x) {
				platforms[i].y += FALLING_SPEED * speed;
			}
			platforms[i].x -= MOVEMENT_SPEED * speed;
			// if a platform has moved past the screen just removed it to the end, no need to create new ones or anything like that
			// can reuse objects we already have
			if (platforms[i].x + PLATFORM_SIZE < 0) {
				platforms[i].x = WINDOW_WIDTH + 10;
				platforms[i].y = WINDOW_HEIGHT - 140 - (rand() % 50);
			}
		}

		// Move background and birds
		for (int i = 0; i < 2; i++) {
			backgrounds[i].x -= BACKGROUND_SPEED;
		}
		for (int i = 0; i < NUM_BIRDS; i++) {
			birds[i].x -= BIRD_SPEED * speed;
		}

		// Move flood
		oscillate_flood(&flood_rect, time, wave_size);

		// Animate sprites
		if (player_rate == 3) {
			player_srcrect.x = ((time / player_rate) % 8) * 32;
			bird_srcrect.x = ((time / player_rate) % 6) * 32;
			flood_srcrect.x = ((time * 60 / 16) % 3) * 320;
			player_rate = 0;
		}

		if (!is_on_the_ground) {
			assets->objects[player_index].texture = player_jump;
			assets->objects[player_index].src_rect->x = 0;
		}
		else {
			assets->objects[player_index].texture = player_texture;
		}
		// Clear the window
		SDL_RenderClear(renderer);
		// Draw images to window
		for (int i = 0; i < total_assets; i++) {
			SDL_RenderCopy(renderer, assets->objects[i].texture, assets->objects[i].src_rect, assets->objects[i].texture_rect);
		}
		SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
		SDL_RenderPresent(renderer);

		if (player_rect.y + player_rect.h >= flood_rect.y) {
			game_over(window, renderer, assets, player_index, full_score_text, player);
			break;
		}

		time++;
		player_rate++;
		send++;
		sprintf(score_nr, "%d", score);
		assert(full_score_text);
		strcpy(full_score_text, score_text);
		strcat(full_score_text, score_nr);
		text = TTF_RenderText_Solid(font, full_score_text, text_color);
		text_texture = SDL_CreateTextureFromSurface(renderer, text);
		// Run game in 60 fps
		SDL_Delay(1000.0 / FRAMES_PER_SECOND);
	}
	free(score_nr);
	free(full_score_text);

	// Clean up textures
	SDL_DestroyTexture(player_texture);
	SDL_DestroyTexture(bird);
	SDL_DestroyTexture(flood);
	SDL_DestroyTexture(background);
}
