#ifndef CHIP8_H
#define CHIP8_H

#include <stdio.h>
#include <cassert>
#include <stdlib.h>
#include <fstream>
#include <iostream>

namespace chip8
{
	typedef unsigned short ushort;
	typedef unsigned char uchar;
	typedef unsigned int uint;

	const unsigned char chip8_fontset[80] =
	{
	  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	  0x20, 0x60, 0x20, 0x20, 0x70, // 1
	  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	const int chip8_screen_width = 64;
	const int chip8_screen_height = 32;
	const int chip8_framebuffer_size = chip8_screen_width * chip8_screen_height;

	const int chip8_key_count = 16;
	const int chip8_stack_size = 16;
	const int chip8_register_count = 16;
	const int chip8_mem_size = 4096;
	const uint chip8_timer_tickrate_ms = 1000 / 60;
	const uint chip8_op_tickrate_ms = 1000 / 60;
	const ushort chip8_digits_ptr = 0x0;
	const uchar chip8_digits_charsize = 0x5;

	enum chip8_external_commands
	{
		// Clear the Screen
		disp_clear,
		wait_for_key
	};

	struct chip8_state
	{
		ushort opcode; //current opcode
		uchar memory[chip8_mem_size]; //mem
		uchar V[chip8_register_count]; //registers
		ushort I; //index register
		ushort pc; //program counter	
		uchar gfx[chip8_framebuffer_size]; //framebuffer

		uchar delay_timer;
		uchar sound_timer;

		ushort stack[chip8_stack_size];
		uchar sp;

		uchar key[chip8_key_count];

		bool draw_flag;

		uint timer_ms;
	};

	const int chip8_index_1d(
		const int x,
		const int y);

	void chip8_initialize(
		chip8_state* state);

	void chip8_load_game(
		chip8_state* state,
		const char* filepath);

	const bool chip8_is_drawflag_set(
		chip8_state* state);

	void chip8_tick_cycle(
		chip8_state* state,
		chip8_external_commands* out_external_commands,
		int* out_external_command_count);
}

#endif
