#ifndef _GAME_
#define _GAME_

void start_game(SDL_Window *window, SDL_Renderer *renderer, int player, char seed);
bool between(int min, int max, int num);
#endif
