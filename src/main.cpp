#include <stdio.h>
#include <stdlib.h>

// https://web.archive.org/web/20210909190432/http://www.obelisk.me.uk/6502/

using Byte = unsigned char;
using Word = unsigned short;

using u32 = unsigned int;

struct Memory
{
	static constexpr u32 MAX_MEM = 1024 * 64;
	Byte Data[MAX_MEM];

	void Initialise()
	{
		for (u32 i = 0; i < MAX_MEM; i++)
		{
			Data[i] = 0;
		}
	}

	/** Read 1 Byte */
	Byte operator[] (u32 Address) const
	{
		// Assert Here Address Is < MAX_MEM
		return Data[Address];
	}

	/** Write 1 Byte */
	Byte& operator[] (u32 Address)
	{
		// Assert Here Address Is < MAX_MEM
		return Data[Address];
	}

	/** Write 2 Bytes */
	void WriteWord(
		Word Value,
		u32 Address,
		u32& Cycles)
	{
		Data[Address]		= Value & 0xFF;
		Data[Address + 1]	= (Value >> 8);
		Cycles -= 2;
	}
};

struct CPU
{
	Word PC;		// Program Counter
	Word SP;		// Stack Pointer

	Byte A, X, Y;	// Registers

	Byte C : 1;		// Status Flags
	Byte Z : 1;
	Byte I : 1;
	Byte D : 1;
	Byte B : 1;
	Byte V : 1;
	Byte N : 1;

	void Reset(Memory& memory)
	{
		PC = 0xFFFC;
		SP = 0x0100;
		C, Z, I, D, B, V, N = 0;
		A, X, Y = 0;
		memory.Initialise();
	}

	Byte FetchByte(u32& Cycles, Memory& memory)
	{
		Byte Data = memory[PC];
		PC++;
		Cycles--;
		return Data;
	}

	Word FetchWord(u32& Cycles, Memory& memory)
	{
		// 6502 Is Little Endian
		Word Data = memory[PC];
		PC++;

		Data |= (memory[PC] << 8);
		PC++;

		Cycles -= 2;

		// If You Wanted To Handle Endianness
		// You Would Have To Swap Bytes Here
		// if (PLATFORM_BIG_ENDIAM)
		// 	SwapBytesInWord(Data)

		return Data;
	}

	Byte ReadByte(
		u32& Cycles,
		Byte Address,
		Memory& memory)
	{
		Byte Data = memory[Address];
		Cycles--;
		return Data;
	}

	// Opcodes
	static constexpr Byte
		INS_LDA_IM = 0xA9,
		INS_LDA_ZP = 0xA5,
		INS_LDA_ZPX = 0xB5,
		INS_JSR = 0x20;

	void LDASetStatus()
	{
		Z = (A == 0);
		N = (A & 0b10000000) > 0;
	}

	void Execute(u32 Cycles, Memory& memory)
	{
		while (Cycles > 0)
		{
			Byte Ins = FetchByte(Cycles, memory);
			switch (Ins)
			{
			case INS_LDA_IM:
			{
				Byte Value = 
					FetchByte(Cycles, memory);
				A = Value;
				LDASetStatus();
			} break;
			case INS_LDA_ZP:
			{
				Byte ZeroPageAddr = 
					FetchByte(Cycles, memory);
				A = ReadByte(
					Cycles, ZeroPageAddr, memory);
				LDASetStatus();
			} break;
			case INS_LDA_ZPX:
			{
				Byte ZeroPageAddr = 
					FetchByte(Cycles, memory);
				ZeroPageAddr += X;
				Cycles--;
				A = ReadByte(
					Cycles, ZeroPageAddr, memory);
				LDASetStatus();
			} break;
			case INS_JSR:
			{
				Word SubAddr =
					FetchWord(Cycles, memory);
				memory.WriteWord(
					PC - 1, SP, Cycles);
				PC = SubAddr;
				Cycles--;
			} break;
			default:
			{
				printf("%s Instruction Not Handled!\n", Ins);
			} break;
			}
		}
	}
};

int main()
{
	Memory memory;
	CPU cpu;
	cpu.Reset(memory);
	// Start - Inline A Little Program
	memory[0xFFFC] = CPU::INS_JSR;
	memory[0xFFFD] = 0x42;
	memory[0xFFFE] = 0x42;
	memory[0x4242] = CPU::INS_LDA_IM;
	memory[0x4243] = 0x84;
	// End - Inline A Little Program
	cpu.Execute(9, memory);
	return 0;
}