#include "chip8.h"
#include <SDL.h>
#include <iostream>

// Display size
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320
#define PIXEL_SIZE 10

Chip8 chip8;

void DrawGraphics(SDL_Renderer* renderer)
{
	// Black background
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// White pixels
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for (int y = 0; y < 32; y++)
	{
		for (int x = 0; x < 64; x++)
		{
			// (y*64)+x calculates the index in the 'gfx' array corresponding to the pixel at coordinates (x,y)
			if (chip8.gfx[(y * 64) + x] == 1)
			{
				SDL_Rect rect = { x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE };
				SDL_RenderFillRect(renderer, &rect);
			}
		}
	}

	SDL_RenderPresent(renderer);
}

void handleKeyEvent(const SDL_Event& event, int state)
{
	switch (event.key.keysym.sym)
	{
		case SDLK_1: chip8.key[0x1] = state; break;
		case SDLK_2: chip8.key[0x2] = state; break;
		case SDLK_3: chip8.key[0x3] = state; break;
		case SDLK_4: chip8.key[0xC] = state; break;

		case SDLK_q: chip8.key[0x4] = state; break;
		case SDLK_w: chip8.key[0x5] = state; break;
		case SDLK_e: chip8.key[0x6] = state; break;
		case SDLK_r: chip8.key[0xD] = state; break;

		case SDLK_a: chip8.key[0x7] = state; break;
		case SDLK_s: chip8.key[0x8] = state; break;
		case SDLK_d: chip8.key[0x9] = state; break;
		case SDLK_f: chip8.key[0xE] = state; break;

		case SDLK_z: chip8.key[0xA] = state; break;
		case SDLK_x: chip8.key[0x0] = state; break;
		case SDLK_c: chip8.key[0xB] = state; break;
		case SDLK_v: chip8.key[0xF] = state; break;
	}
}

int main(int argc, char** argv)
{
	// If less than 2 arguments were passed to the program
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
		return 1;
	}

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Taha CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	// Load the game into the memory
	chip8.LoadGame(argv[1]);

	bool quit = false;
	SDL_Event e;

	while (!quit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT) { quit = true; }
			else if (e.type == SDL_KEYDOWN) { handleKeyEvent(e, 1); }
			else if (e.type == SDL_KEYUP) { handleKeyEvent(e, 0); }
		}

		chip8.EmulateCycle();

		if (chip8.drawFlag)
		{
			DrawGraphics(renderer);
			chip8.drawFlag = false;
		}

		// ~60Hz
		SDL_Delay(16);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}