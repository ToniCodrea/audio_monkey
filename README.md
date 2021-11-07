# audio_monkey

This project was built as part of the extension for the Imperial College London Arm11 emulator and assembler project. Team members:
- Antonio Codrea
- James Matthew Young
- Lucas Rhadakishun
- Lynna Xie

This project is a simple 2D network multiplayer game that uses one of the player's voice as the main input. This game was built in C using SDL2, OpenAL and the Winsock networking libraries over the course of the last 2 days of the project. The game follows a monkey as it jumps around through a forest that is being flooded. One player controls the monkey's jump using the `spacebar` key, while the other player controls the height of the platforms by raising their voice. The game's objective is to obtain as high a score as possible. Whenever the score increases, the speed of the game also increases, making for a challenging experience for both players. 

The game can be built from the files in `src` and `assets`. If you simply wish to run the game, the `build.rar` file contains the game executable alongside the necessary libraries and assets. 

The game can currently only be run on Windows, over a local network. 

In order to start playing, the host (the player who will control the monkey) must press the monkey-shaped icon on the main menu. After this, the guest (the voice player) should input the host's IP in the text field and press the microhpone button. This will start calibrating the quiet and netural levels for the voice player, after which the game begins. In case of game-over, if both players agree on playing again the game will restart. 

A short demo of the game from our project presentation is available here: https://youtu.be/rth3QoDg9k4
