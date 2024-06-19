#include "Chip8.h"
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>

Chip8::Chip8()
{

}

Chip8::~Chip8()
{

}

// Initialise registers and memory once
void Chip8::Initialise()
{
	pc	   = 0x200; // Program counter starts at 0x200
	opcode = 0;		// Reset current opcode
	I	   = 0;		// Reset index register
	sp	   = 0;		// Reset stack pointer

	// Clear display
	for (int i = 0; i < 2048; i++)
	{
		gfx[i] = 0;
	}

	// Clear stack
	for (int i = 0; i < 16; i++)
	{
		stack[i] = 0;
	}

	// Clear registers V0-VR
	for (int i = 0; i < 16; i++)
	{
		key[i] = V[i] = 0;
	}

	// Clear memory
	for (int i = 0; i < 4096; i++)
	{
		memory[i] = 0;
	}

	// Load fontsetd
	for (int i = 0; i < 80; i++)
	{
		memory[i] = chip8_fontset[i];
	}

	// Reset timers
	delayTimer = 0;
	soundTimer = 0;

	// Clear screen once
	drawFlag = true;

	srand(time(NULL));
}

void Chip8::EmulateCycle()
{
	// Fetch Opcode
	opcode = memory[pc] << 8 | memory[pc + 1];

	// Decode Opcode
	switch(opcode & 0xF000)
	{
		case 0x0000:
			switch (opcode & 0x000F)
			{
				case 0x0000: // 0x00E0: Clears the screen
					for (int i = 0; i < 2048; i++)
					{
						gfx[i] = 0x0;
					}
					drawFlag = true;
					pc += 2;
					break;

				case 0x000E: // 0x00EE: Returns from subroutine
					sp--;
					pc = stack[sp];
					pc += 2;
					break;

				default:
					printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
			}
			break;

		case 0x1000: // 0x1NNN: Jumps to address NNN
			pc = opcode & 0x0FFF;
			break;

		case 0x2000: // 0x2NNN: Calls subroutine at NNN
			stack[sp] = pc;
			sp++;
			pc = opcode & 0x0FFF;
			break;

		case 0x3000: // 0x3XNN: Skips the next instrucction if VX equals NN
			if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) { pc += 4; }
			else { pc += 2; }
			break;

		case 0x4000: // 0x4XNN: Skips the next instruction if VX does not equal NN
			if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) { pc += 4; }
			else { pc += 2; }
			break;

		case 0x5000: // 0x5XY0: Skips the next instruction if VX equals VY
			if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) { pc += 4; }
			else { pc += 2; }
			break;

		case 0x6000: // 0x6XNN: Sets VX to NN
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
			pc += 2;
			break;

		case 0x7000: // 0x7XNN: Adds NN to VX (carry flag is not changed)
			V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
			pc += 2;
			break;

		case 0x8000:
			switch (opcode & 0x000F)
			{
				case 0x0000: // 0x8XY0: Sets VX to the value of VY
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0001: // 0x8XY1: Sets VX to VX or VY (OR operation)
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] | V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0002: // 0x8XY2: Sets VX to VX and VY (AND operation)
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] & V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0003: // 0x8XY3: Sets VX to VX xor VY
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] ^ V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0004: // 0x8XY4: Adds VY to VX. VF is set to 1 when there's an overflow, 0 otherwise
					if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) { V[0xF] = 1; }
					else { V[0xF] = 0; }

					V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0005: // 0x8XY5: VY is subtracted from VX. VF is set to 0 when there's an underflow, 1 otherwise
					if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) { V[0xF] = 0; }
					else { V[0xF] = 1; }

					V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0006: // 0x8XY6: Shifts VX to the right by 1. Stores LSB of VX to VF before shift
					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;

					V[(opcode & 0x0F00) >> 8] >>= 1;
					pc += 2;
					break;

				case 0x0007: // 0x8XY7: Sets VX to VY minus VX. VF set to 0 when underflow, 1 otherwise
					if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) { V[0xF] = 0; }
					else { V[0xF] = 1; }

					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;

				case 0x000E: // 0x8XYE: Shifts VX to the left by 1. Sets VF to 1 if MSB of VX was set prior to shift, 0 otherwise
					V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;

					V[(opcode & 0x0F00) >> 8] <<= 1;
					pc += 2;
					break;

				default:
					printf("Unknon opcode [0x8000]: 0x%X\n", opcode);
			}
			break;

		case 0x9000: // 0x9XY0: Skips next instruction if VX does not equal VY
			if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) { pc += 4; }
			else { pc += 2; }
			break;

		case 0xA000: // 0xANNN: Sets I to the address NNN
			I = opcode & 0x0FFF;
			pc += 2;
			break;

		case 0xB000: // 0xBNNN: Jumps to the address NNN plus V0
			pc = (opcode & 0x0FFF) + V[0];
			break;

		case 0xC000: // 0xCXNN: Sets VX to the result of a bitwise AND operation on a random number and NN
			V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
			pc += 2;
			break;

		case 0xD000: // 0xDXYN: Draw a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
					 //			Each row of 8 pixels is read as bit-coded starting from memory location I; I value does not change after execution of instruction.
					 //			VF is set to 1 if any screen pixels are flipped from set to unset when sprite is drawn, 0 otherwise
		{
			unsigned short x = V[(opcode & 0x0F00) >> 8];
			unsigned short y = V[(opcode & 0x00F0) >> 4];
			unsigned short height = opcode & 0x000F;
			unsigned short pixel;

			for (int i = 0; i < height; i++) {
				std::cout << std::hex << (int)memory[I + i] << " ";
			}
			std::cout << std::dec << std::endl;

			V[0xF] = 0;

			for (int yline = 0; yline < height; yline++)
			{
				pixel = memory[I + yline];
				for (int xline = 0; xline < 8; xline++)
				{
					if ((pixel & (0x80 >> xline)) != 0)
					{
						if (gfx[(x + xline + ((y + yline) * 64))] == 1)
						{
							V[0xF] = 1;
						}
						gfx[x + xline + ((y + yline) * 64)] ^= 1;
					}
				}
			}

			drawFlag = true;
			pc += 2;
		}
			break;

		case 0xE000:
			switch (opcode & 0x00FF)
			{
				case 0x009E: // 0xEX9E: Skips the next instruction if key stored in VX is pressed
					if (key[V[(opcode & 0x0F00) >> 8]] != 0) { pc += 4; }
					else { pc += 2; }
					break;

				case 0x00A1: // 0xEXA1: Skips the next instruction if key stored in VX is not pressed
					if (key[V[(opcode & 0x0F00) >> 8]] == 0) { pc += 4; }
					else { pc += 2; }
					break;

				default:
					printf("Unknon opcode [0xE000]: 0x%X\n", opcode);
			}
			break;

		case 0xF000:
			switch (opcode & 0x00FF)
			{
				case 0x0007: // 0xFX07: Sets VX to the value of the delay timer
					V[(opcode & 0x0F00) >> 8] = delayTimer;
					pc += 2;
					break;

				case 0x000A: // 0xFX0A: A key press is awaited, and then stored in VX
				{
					bool keyPress = false;

					for (int i = 0; i < 16; i++)
					{
						if (key[i] != 0)
						{
							V[(opcode & 0x0F00) >> 8] = i;
							keyPress = true;
						}
					}

					if (!keyPress) { return; }

					pc += 2;
				}
					break;

				case 0x0015: // 0xFX15: Sets delay timer to VX
					delayTimer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;

				case 0x0018: // 0xFX18: Sets sound timer to VX
					soundTimer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;

				case 0x001E: // 0xFX1E: Adds VX to I. VF is not affected
					I += V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;

				case 0x0029: // 0xFX29: Sets I to the location of the sprite for the character in VX
					I = V[(opcode & 0x0F00) >> 8] * 0x5;
					pc += 2;
					break;

				case 0x0033: // 0xFX33: Stores binary-coded decimal representation of VX
					memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
					memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
					memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
					pc += 2;
					break;

				case 0x0055: // 0xFX55: Stores from V0 to VX (including VX) in memory
					for (int i = 0; i < ((opcode & 0x0F00) >> 8); i++)
					{
						memory[I + i] = V[i];
					}

					// From the original interpreter, I is left inremented after this instruction has been executed
 					I += ((opcode & 0x0F00) >> 8) + 1;
					pc += 2;
					break;

				case 0x0065: // 0xFX65: Fills from V0 to VX (including VX) from memory
					// Execute opcode
					for (int i = 0; i < ((opcode & 0x0F00) >> 8); i++)
					{
						V[i] = memory[I + i];
					}

					// From the original interpreter, I is left inremented after this instruction has been executed
					I += ((opcode & 0x0F00) >> 8) + 1;
					pc += 2;
					break;

				default:
					printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
			}
			break;

		default:
			printf("Unknown opcode: 0x%X\n", opcode);
	}

	// Update timers
	if (delayTimer > 0) { delayTimer--; }

	if (soundTimer > 0)
	{
		if (soundTimer == 1)
			printf("BEEP!\n");
		soundTimer--;
	}

}

bool Chip8::LoadGame(const char* game)
{
	// Initialise
	Initialise();
	std::cout << "Loading: " << game << std::endl;

	FILE* pFile;
	errno_t err;

	// Open file
	if ((err = fopen_s(&pFile, game, "rb")) != 0)
	{
		fputs("File error", stderr);
		return false;
	}

	// Check file size
	fseek(pFile, 0, SEEK_END);
	long lSize = ftell(pFile);
	rewind(pFile);
	std::cout << "Filesize: " << lSize << std::endl;

	// Allocate memory to contain the whole file
	char* buffer = (char*)malloc(sizeof(char) * lSize);
	if (buffer == NULL)
	{
		fputs("Memory error", stderr);
		return false;
	}

	// Copy file into buffer
	size_t result = fread(buffer, 1, lSize, pFile);
	if (result != lSize)
	{
		fputs("Reading error", stderr);
		return false;
	}

	// Copy buffer into chip8 memory
	if ((4098 - 512) > lSize)
	{
		// Start filling the memory at location: 0x200 == 512
		for (int i = 0; i < lSize; i++)
		{
			memory[i + 512] = buffer[i];
		}
	}
	else
		std::cout << "Error: ROM too big for memory";

	// Close files, free buffer
	fclose(pFile);
	free(buffer);

	return true;
}