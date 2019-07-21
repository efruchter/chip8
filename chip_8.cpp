#include "chip_8.h"

namespace chip8
{
	const int chip8_index_1d(const int x, const int y)
	{
		return x + (y * chip8_screen_width);
	}

	void chip8_initialize(chip8_state* state)
	{
		srand(0);

		state->pc = 0x200;
		state->opcode = 0;
		state->I = 0;
		state->sp = 0;

		for (int i = 0; i < chip8_framebuffer_size; i++)
			state->gfx[i] = 0;

		state->draw_flag = 0;

		for (int i = 0; i < chip8_stack_size; i++)
			state->stack[i] = 0;

		for (int i = 0; i < chip8_register_count; i++)
			state->V[i] = 0;

		for (int i = 0; i < chip8_mem_size; i++)
			state->memory[i] = 0;

		for (int i = 0; i < 80; ++i)
			state->memory[chip8_digits_ptr + i] = chip8_fontset[i];

		state->delay_timer = 0;
		state->sound_timer = 0;
		state->timer_ms = 0;
	}

	void chip8_load_game(chip8_state* state, const char* filepath)
	{
		using namespace std;

		ifstream fl(filepath, ios::binary | ios::in);
		assert(fl.is_open());

		fl.seekg(0, ios::end);
		size_t len = fl.tellg();
		char* ret = new char[len];
		fl.seekg(0, ios::beg);
		fl.read(ret, len);
		fl.close();

		for (int i = 0; i < len; i++)
			state->memory[0x200 + i] = ret[i];

		delete[] ret;
	}

	const bool chip8_is_drawflag_set(chip8_state* state)
	{
		return state->draw_flag;
	}

	void chip8_tick_cycle(
		chip8_state* c8,
		chip8_external_commands* out_external_commands,
		int* out_external_command_count)
	{
		*out_external_command_count = 0;
		c8->draw_flag = false;

		if (c8->pc < 0 || c8->pc >= (chip8_mem_size - 1))
		{
			printf("Program Counter is out of memory bounds: %d\n", c8->pc);
			return;
		}

		// Fetch opcode
		const ushort opcode = (c8->memory[c8->pc] << 8) | (c8->memory[c8->pc + 1]);
		c8->opcode = opcode;

		//printf("Opcode: 0x%X\n", opcode);

		// Decode opcode
		switch (opcode & 0xF000)
		{
		case 0x0000:
		{
			switch (opcode & 0x000F)
			{
			case 0x0000: // CLS
				out_external_commands[*out_external_command_count++] = disp_clear;

				for (int i = 0; i < chip8_framebuffer_size; i++)
					c8->gfx[i] = 0;

				c8->pc += 2;
				c8->draw_flag = true;
				break;
			case 0x000E: // RET
				c8->pc = c8->stack[--(c8->sp)];
				break;
			default: // IGNORE
				c8->pc += 2;
				break;
			}
			break;
		}
		case 0x1000: // 1nnn - JP addr
			c8->pc = (opcode & 0x0FFF);
			break;
		case 0x2000: // 2nnn - CALL addr
			c8->stack[c8->sp++] = c8->pc + 2;
			c8->pc = opcode & 0x0FFF;
			break;
		case 0x3000: // 3xkk - SE Vx, byte
		{
			uchar x = (opcode & 0x0F00) >> 8;
			uchar vx = c8->V[x];
			uchar nn = (opcode & 0x00FF);
			if (vx == nn)
				c8->pc += 4;
			else
				c8->pc += 2;
			break;
		}
		case 0x4000: // 4xkk - SNE Vx, byte
		{
			uchar x = (opcode & 0x0F00) >> 8;
			uchar vx = c8->V[x];
			uchar nn = (opcode & 0x00FF);
			if (vx != nn)
				c8->pc += 4;
			else
				c8->pc += 2;
			break;
		}
		case 0x5000: // 5xy0 - SE Vx, Vy
		{
			uchar x = (opcode & 0x0F00) >> 8;
			uchar y = (opcode & 0x00F0) >> 4;
			uchar vx = c8->V[x];
			uchar vy = c8->V[y];
			if (vx == vy)
				c8->pc += 4;
			else
				c8->pc += 2;
			break;
		}
		case 0x6000: // 6xkk - LD Vx, byte
		{
			uchar x = (opcode & 0x0F00) >> 8;
			c8->V[x] = (opcode & 0x00FF);
			c8->pc += 2;
			break;
		}
		case 0x7000: // 7xkk - ADD Vx, byte
		{
			c8->V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
			c8->pc += 2;
			break;
		}
		case 0x8000:
		{
			switch (opcode & 0x000F)
			{
			case 0x0000: // 8xy0 - LD Vx, Vy
			{
				c8->V[(opcode & 0x0F00) >> 8] = c8->V[(opcode & 0x00F0) >> 4];
				c8->pc += 2;
				break;
			}
			case 0x0001: //8xy1 - OR Vx, Vy
			{
				c8->V[(opcode & 0x0F00) >> 8] |= c8->V[(opcode & 0x00F0) >> 4];
				c8->pc += 2;
				break;
			}
			case 0x0002: //8xy2 - AND Vx, Vy
			{
				c8->V[(opcode & 0x0F00) >> 8] &= c8->V[(opcode & 0x00F0) >> 4];
				c8->pc += 2;
				break;
			}
			case 0x0003: // 8xy3 - XOR Vx, Vy
			{
				c8->V[(opcode & 0x0F00) >> 8] ^= c8->V[(opcode & 0x00F0) >> 4];
				c8->pc += 2;
				break;
			}
			case 0x0004: // 8xy4 - ADD Vx, Vy
			{
				if (c8->V[(opcode & 0x00F0) >> 4] > (0xFF - c8->V[(opcode & 0x0F00) >> 8]))
					c8->V[0xF] = 1;
				else
					c8->V[0xF] = 0;
				c8->V[(opcode & 0x0F00) >> 8] += c8->V[(opcode & 0x00F0) >> 4];
				c8->pc += 2;
				break;
			}
			case 0x0005: //8xy5 - SUB Vx, Vy
			{
				if (c8->V[(opcode & 0x0F00) >> 8] > c8->V[(opcode & 0x00F0) >> 4])
					c8->V[0xF] = 1;
				else
					c8->V[0xF] = 0;
				c8->V[(opcode & 0x0F00) >> 8] -= c8->V[(opcode & 0x00F0) >> 4];
				c8->pc += 2;
				break;
			}
			case 0x0006: // 8xy6 - SHR Vx {, Vy}
			{
				c8->V[0xF] = (c8->V[(opcode & 0x0F00) >> 8] & 1);
				c8->V[(opcode & 0x0F00) >> 8] /= 2;
				c8->pc += 2;
				break;
			}
			case 0x0007: // 8xy7 - SUBN Vx, Vy
			{
				const uchar x = (opcode & 0x0F00) >> 8;
				const uchar y = (opcode & 0x00F0) >> 4;
				const uchar vx = c8->V[x];
				const uchar vy = c8->V[y];
				if (vy > vx)
					c8->V[0xF] = 1;
				else
					c8->V[0xF] = 0;
				c8->V[x] = c8->V[y] - c8->V[x];
				c8->pc += 2;
				break;
			}
			case 0x000E: // 8xyE - SHL Vx {, Vy}
			{
				c8->V[0xF] = ((c8->V[(opcode & 0x0F00) >> 8] & 128) == 0) ? 0 : 1;
				c8->V[(opcode & 0x0F00) >> 8] *= 2;
				c8->pc += 2;
				break;
			}
			default:
				printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
				break;
			}
			break;
		}
		case 0x9000: //9xy0 - SNE Vx, Vy
		{
			const uchar x = (opcode & 0x0F00) >> 8;
			const uchar y = (opcode & 0x00F0) >> 4;
			const uchar vx = c8->V[x];
			const uchar vy = c8->V[y];
			if (vx != vy)
				c8->pc += 4;
			else
				c8->pc += 2;
			break;
		}
		case 0xA000: // Annn - LD I, addr
		{
			c8->I = opcode & 0x0FFF;
			c8->pc += 2;
			break;
		}
		case 0xB000: //Bnnn - JP V0, addr
		{
			c8->pc = (opcode & 0x0FFF) + c8->V[0x0];
			break;
		}
		case 0xC000: //Cxkk - RND Vx, byte
		{
			c8->V[(opcode & 0x0F00) >> 8] = (((uint)rand()) % 255) & (opcode & 0x00FF);
			c8->pc += 2;
			break;
		}
		case 0xD000: //Dxyn - DRW Vx, Vy, nibble
		{
			unsigned short x = c8->V[(opcode & 0x0F00) >> 8];
			unsigned short y = c8->V[(opcode & 0x00F0) >> 4];
			unsigned short height = opcode & 0x000F;
			unsigned short pixel;

			c8->V[0xF] = 0;
			for (int yline = 0; yline < height; yline++)
			{
				pixel = c8->memory[c8->I + yline];
				for (int xline = 0; xline < 8; xline++)
				{
					if ((pixel & (0x80 >> xline)) != 0)
					{
						if (c8->gfx[(x + xline + ((y + yline) * 64))] == 1)
							c8->V[0xF] = 1;
						c8->gfx[x + xline + ((y + yline) * 64)] ^= 1;
					}
				}
			}

			c8->draw_flag = true;
			c8->pc += 2;
			break;
		}
		case 0xE000:
		{
			switch (opcode & 0x000F)
			{
			case 0x000E: //Ex9E - SKP Vx
			{
				if (c8->key[c8->V[(opcode & 0x0F00) >> 8]])
					c8->pc += 4;
				else
					c8->pc += 2;
				break;
			}
			case 0x0001: //ExA1 - SKNP Vx:w
			{
				if (c8->key[c8->V[(opcode & 0x0F00) >> 8]])
					c8->pc += 2;
				else
					c8->pc += 4;
				break;
			}

			default:
				printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
				break;
			}

			break;
		}
		case 0xF000:
		{
			switch (opcode & 0x00FF)
			{
			case 0x0007: // Fx07 - LD Vx, DT
			{
				c8->V[(opcode & 0x0F00) >> 8] = c8->delay_timer;
				c8->pc += 2;
				break;
			}
			case 0x000A: //Fx0A - LD Vx, K
			{
				bool key = false;
				for (uchar i = 0; i < chip8_key_count && !key; i++)
				{
					if (c8->key[i])
					{
						key = true;
						c8->V[(opcode & 0x0F00) >> 8] = i;
						c8->pc += 2;
					}
				}

				if (!key)
					out_external_commands[(*out_external_command_count)++] = wait_for_key;

				break;
			}
			case 0x0015: //Fx15 - LD DT, Vx
			{
				c8->delay_timer = c8->V[(opcode & 0x0F00) >> 8];
				c8->pc += 2;
				break;
			}
			case 0x0018: //Fx18 - LD ST, Vx
			{
				c8->sound_timer = c8->V[(opcode & 0x0F00) >> 8];
				c8->pc += 2;
				break;
			}
			case 0x001E: //Fx1E - ADD I, Vx
			{
				c8->I += c8->V[(opcode & 0x0F00) >> 8];
				c8->pc += 2;
				break;
			}
			case 0x0029: //Fx29 - LD F, Vx
			{
				c8->I = chip8_digits_ptr + (chip8_digits_charsize * c8->V[(opcode & 0x0F00) >> 8]);
				c8->pc += 2;
				break;
			}
			case 0x0033: //Fx33 - LD B, Vx
			{
				c8->memory[c8->I] = c8->V[(opcode & 0x0F00) >> 8] / 100;
				c8->memory[c8->I + 1] = (c8->V[(opcode & 0x0F00) >> 8] / 10) % 10;
				c8->memory[c8->I + 2] = (c8->V[(opcode & 0x0F00) >> 8] % 100) % 10;
				c8->pc += 2;
				break;
			}
			case 0x0055: //Fx55 - LD [I], Vx
			{
				uchar x = (opcode & 0x0F00) >> 8;
				for (uchar i = 0; i <= x; i++)
					c8->memory[c8->I + i] = c8->V[i];
				c8->pc += 2;
				break;
			}
			case 0x0065: //Fx65 - LD Vx, [I]
			{
				uchar x = (opcode & 0x0F00) >> 8;
				for (uchar i = 0; i <= x; i++)
					c8->V[i] = c8->memory[c8->I + i];
				c8->pc += 2;
				break;
			}
			default:
			{
				printf("Unknown opcode: 0x%X\n", opcode);
				break;
			}
			}
			break;
		}
		default:
		{
			printf("Unknown opcode: 0x%X\n", opcode);
			break;
		}
		}

		// Update timers

		if (c8->delay_timer > 0)
			c8->delay_timer--;

		if (c8->sound_timer > 0)
			c8->sound_timer--;
	}
}

