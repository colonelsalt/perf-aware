#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ArrayCount(X) (sizeof(X) / sizeof((X)[0]))
#define Assert(X) {if (!(X)) __debugbreak();}

struct segmented_access;

#include "sim86_shared.h"
#include "sim86_decode.h"
#include "decode_print.cpp"

static constexpr u32 MEMORY_SIZE = 65'536; // 64kB
static u8 s_Memory[MEMORY_SIZE];

void LoadFile(char* FileName, u8* Buffer, u32* OutFileSize)
{
	FILE* File = fopen(FileName, "rb");
	if (File)
	{
		u32 Start = ftell(File);
		fseek(File, 0, SEEK_END);
		*OutFileSize = (u32)(ftell(File)) - Start;
		fseek(File, 0, SEEK_SET);

		size_t BytesRead = fread(Buffer, 1, *OutFileSize, File);
		Assert(BytesRead == *OutFileSize);
		fclose(File);
	}
	else
	{
		printf("ERROR: Couldn't open file %s\n", FileName);
		Assert(false);
	}
}

void DumpMemory(u8* Memory, u32 MemorySize, char* FileName)
{
	FILE* File = fopen(FileName, "wb");
	if (File)
	{
		size_t BytesWritten = fwrite(Memory, sizeof(u8), MemorySize, File);
		Assert(BytesWritten == MemorySize);
	}
	else
	{
		printf("Failed to create memory dump file: %s\n", FileName);
		Assert(false);
	}
	fclose(File);
	printf("\nWrote memory dump to %s\n", FileName);
}

enum reg_flags
{
    Flag_ZF = (1 <<  6),
    Flag_SF = (1 <<  7),
};

union reg_contents
{
	struct
	{
		u8 Low;
		u8 High;
	};
	u16 Extended;
};

void PrintFlags(u16 RegFlags)
{
	if (RegFlags & Flag_ZF)
	{
		printf("Z");
	}
	if (RegFlags & Flag_SF)
	{
		printf("S");
	}
}

void PrintRegContents(reg_contents* Regs)
{
	printf("\nFinal registers: \n");
	for (u32 i = Register_a; i < Register_flags; ++i)
	{
		register_access Reg = {};
		Reg.Index = i;
		Reg.Count = 2;

		if (Regs[i].Extended != 0)
		{
			printf("\t%s: 0x%04x (%d)\n", GetRegName(Reg), Regs[i].Extended, Regs[i].Extended);
		}
	}
	
	printf("   flags: ");
	PrintFlags(Regs[Register_flags].Extended);
}

void PrintRegDiff(reg_contents* RegsCurrent, reg_contents* RegsOld)
{
	for (u32 i = Register_a; i < Register_flags; i++)
	{
		u16 OldVal = RegsOld[i].Extended;
		u16 CurrVal = RegsCurrent[i].Extended;
		if (OldVal != CurrVal)
		{
			register_access Reg = {};
			Reg.Index = i;
			Reg.Count = 2;

			printf("%s:0x%x->0x%x ", GetRegName(Reg), OldVal, CurrVal);
		}
	}
	u16 OldFlags = RegsOld[Register_flags].Extended;
	u16 CurrFlags = RegsCurrent[Register_flags].Extended;

	if (OldFlags != CurrFlags)
	{
		printf(" flags:");
		PrintFlags(OldFlags);
		printf("->");
		PrintFlags(CurrFlags);
	}
}

void SetFlags8(u8 Value, u16* OutFlags)
{
	if (Value == 0)
	{
		*OutFlags |= Flag_ZF;
	}
	else
	{
		*OutFlags &= ~Flag_ZF;
	}
	if (Value & (1 << 7))
	{
		*OutFlags |= Flag_SF;
	}
	else
	{
		*OutFlags &= ~Flag_SF;
	}
}

void SetFlags16(u16 Value, u16* OutFlags)
{
	if (Value == 0)
	{
		*OutFlags |= Flag_ZF;
	}
	else
	{
		*OutFlags &= ~Flag_ZF;
	}
	if (Value & (1 << 15))
	{
		*OutFlags |= Flag_SF;
	}
	else
	{
		*OutFlags &= ~Flag_SF;
	}
}


void Perform8BitOp(u8* Dest, u8 Source, u16* OutFlags, operation_type Op)
{
	switch (Op)
	{
		case Op_mov:
		{
			*Dest = Source;
		} break;
		case Op_add:
		{
			*Dest += Source;
			SetFlags8(*Dest, OutFlags);
		} break;
		case Op_sub:
		{
			*Dest -= Source;
			SetFlags8(*Dest, OutFlags);
		} break;
		case Op_cmp:
		{
			u8 Value = *Dest - Source;
			SetFlags8(Value, OutFlags);
		} break;
	}
}

void Perform16BitOp(u16* Dest, u16 Source, u16* OutFlags, operation_type Op)
{
	switch (Op)
	{
		case Op_mov:
		{
			*Dest = Source;
		} break;
		case Op_add:
		{
			*Dest += Source;
			SetFlags16(*Dest, OutFlags);
		} break;
		case Op_sub:
		{
			*Dest -= Source;
			SetFlags16(*Dest, OutFlags);
		} break;
		case Op_cmp:
		{
			u16 Value = *Dest - Source;
			SetFlags16(Value, OutFlags);
		} break;
	}
}

void RegToRegOp(register_access SourceReg, register_access DestReg, reg_contents* RegContents, operation_type Op)
{
	Assert(SourceReg.Index > Register_none && SourceReg.Index < Register_count);
	Assert(DestReg.Index > Register_none && DestReg.Index < Register_count);

	reg_contents* SourceVal = &RegContents[SourceReg.Index];
	reg_contents* DestVal = &RegContents[DestReg.Index];
	u16* Flags = &RegContents[Register_flags].Extended;

	if (SourceReg.Count == 2)
	{
		Perform16BitOp(&DestVal->Extended, SourceVal->Extended, Flags, Op);
	}
	else
	{
		Assert(SourceReg.Offset <= 1);
		Assert(DestReg.Count < 2 && DestReg.Offset <= 1);

		if (SourceReg.Offset == 0)
		{
			if (DestReg.Offset == 0)
			{
				Perform8BitOp(&DestVal->Low, SourceVal->Low, Flags, Op);
			}
			else
			{
				Perform8BitOp(&DestVal->High, SourceVal->Low, Flags, Op);
			}
		}
		else
		{
			if (DestReg.Offset == 0)
			{
				Perform8BitOp(&DestVal->Low, SourceVal->High, Flags, Op);
			}
			else
			{
				Perform8BitOp(&DestVal->High, SourceVal->High, Flags, Op);
			}
		}
	}
}

void ImmToRegOp(immediate Immediate, register_access DestReg, reg_contents* RegContents, operation_type Op)
{
	Assert(DestReg.Index > Register_none && DestReg.Index < Register_count);

	u16* Flags = &RegContents[Register_flags].Extended;
	reg_contents* DestVal = &RegContents[DestReg.Index];
	if (DestReg.Count == 2)
	{
		Perform16BitOp(&DestVal->Extended, (u16)Immediate.Value, Flags, Op);
	}
	else
	{
		Assert(DestReg.Offset <= 1);
		
		u8 ImmVal = (u8)Immediate.Value;
		if (DestReg.Offset == 0)
		{
			Perform8BitOp(&DestVal->Low, ImmVal, Flags, Op);
		}
		else
		{
			Perform8BitOp(&DestVal->High, ImmVal, Flags, Op);
		}
	}
}

u16 GetEffectiveAddress(effective_address_expression Address, reg_contents* RegContents)
{
	u16 Result = Address.Displacement;
	for (int i = 0; i < 2; i++)
	{
		register_access EffReg = Address.Terms[i].Register;
		if (EffReg.Index != Register_none)
		{
			Result += RegContents[EffReg.Index].Extended;
		}
	}
	return Result;
}

u32 Simulate(instruction Instruction, reg_contents* RegContents, u8* Memory)
{
	RegContents[Register_ip].Extended += Instruction.Size;

	if (Instruction.Operands[0].Type == Operand_Memory || Instruction.Operands[1].Type == Operand_Memory)
	{
		u16* Flags = &RegContents[Register_flags].Extended;
		if (Instruction.Operands[0].Type == Operand_Memory)
		{
			// Storing to memory
			u16 MemoryAddr = GetEffectiveAddress(Instruction.Operands[0].Address, RegContents);
			u8* MemoryLoc = Memory + MemoryAddr;

			if (Instruction.Operands[1].Type == Operand_Immediate)
			{
				// Immediate to memory
				if (Instruction.Flags & Inst_Wide)
				{
					u16 Immval = (u16)Instruction.Operands[1].Immediate.Value;
					Perform16BitOp((u16*)MemoryLoc, Immval, Flags, Instruction.Op);
				}
				else
				{
					u8 Immval = (u8)Instruction.Operands[1].Immediate.Value;
					Perform8BitOp(MemoryLoc, Immval, Flags, Instruction.Op);
				}
			}
			else
			{
				// Register to memory
				Assert(Instruction.Operands[1].Type == Operand_Register);
				register_access SourceReg = Instruction.Operands[1].Register;

				if (SourceReg.Count == 2)
				{
					u16 RegVal = RegContents[SourceReg.Index].Extended;
					Perform16BitOp((u16*)MemoryLoc, RegVal, Flags, Instruction.Op);
				}
				else
				{
					Assert(SourceReg.Offset <= 1);
					u8 RegVal;
					if (SourceReg.Offset == 0)
					{
						RegVal = RegContents[SourceReg.Index].Low;
					}
					else
					{
						RegVal = RegContents[SourceReg.Index].High;
					}
					Perform8BitOp(MemoryLoc, RegVal, Flags, Instruction.Op);
				}
			}
		}
		else
		{
			// Loading from memory
			Assert(Instruction.Operands[0].Type == Operand_Register);
			Assert(Instruction.Operands[1].Type == Operand_Memory);

			register_access DestReg = Instruction.Operands[0].Register;

			u16 MemoryAddr = GetEffectiveAddress(Instruction.Operands[1].Address, RegContents);
			u8* MemoryLoc = Memory + MemoryAddr;

			if (Instruction.Flags & Inst_Wide)
			{
				Assert(DestReg.Count == 2);
				Perform16BitOp(&RegContents[DestReg.Index].Extended, *((u16*)MemoryLoc), Flags, Instruction.Op);
			}
			else
			{
				Assert(DestReg.Offset <= 1);
				u8* DestVal;
				if (DestReg.Offset == 0)
				{
					DestVal = &RegContents[DestReg.Index].Low;
				}
				else
				{
					DestVal = &RegContents[DestReg.Index].High;
				}
				Perform8BitOp(DestVal, *MemoryLoc, Flags, Instruction.Op);
			}
		}
	}
	
	
	if (Instruction.Op < Op_jmp)
	{
		if (Instruction.Operands[0].Type == Operand_Register)
		{
			register_access DestReg = Instruction.Operands[0].Register;
			if (Instruction.Operands[1].Type == Operand_Immediate)
			{
				immediate Immediate = Instruction.Operands[1].Immediate;
				ImmToRegOp(Immediate, DestReg, RegContents, Instruction.Op);
			}
			else if (Instruction.Operands[1].Type == Operand_Register)
			{
				register_access SourceReg = Instruction.Operands[1].Register;
				RegToRegOp(SourceReg, DestReg, RegContents, Instruction.Op);
			}
			else
			{
				// TODO...
			}
		}
		else
		{
			// TODO...
		}
	}
	else
	{
		// We assume this is a jump instruction of some kind
		if (Instruction.Op == Op_loop)
		{
			Assert(Instruction.Operands[0].Type == Operand_Immediate);
			if (--RegContents[Register_c].Extended != 0)
			{
				s32 JumpOffset = Instruction.Operands[0].Immediate.Value;
				// This seems to work, but I'm slightly concerned about the signed/unsigned mixing...
				RegContents[Register_ip].Extended += (u16)JumpOffset;
			}
		}
		else if (Instruction.Op == Op_jne)
		{
			Assert(Instruction.Operands[0].Type == Operand_Immediate);

			if (!(RegContents[Register_flags].Extended & Flag_ZF))
			{
				s32 JumpOffset = Instruction.Operands[0].Immediate.Value;
				RegContents[Register_ip].Extended += (u16)JumpOffset;
			}
		}
	}

	return RegContents[Register_ip].Extended;
}

struct list_node
{
	instruction Instruction;
	list_node* Next;
};

int main(int ArgCount, char** ArgV)
{
    u32 Version = Sim86_GetVersion();
    if(Version != SIM86_VERSION)
    {
        printf("ERROR: Header file version doesn't match DLL.\n");
        return -1;
    }

	if (ArgCount > 4)
	{
		printf("Usage, homie\n");
		return 1;
	}

	char* FileName;
	b32 ShouldExecute = false;
	b32 ShouldDump = false;
	for (int i = 1; i < ArgCount; i++)
	{
		if (strcmp(ArgV[i], "-exec") == 0)
		{
			ShouldExecute = true;
		}
		else if (strcmp(ArgV[i], "-dump") == 0)
		{
			ShouldDump = true;
		}
		else
		{
			FileName = ArgV[i];
		}
	}

	reg_contents RegisterContents[Register_count] = {0};
	reg_contents OldRegContents[Register_count] = {0};

	u32 FileSize;
	LoadFile(FileName, s_Memory, &FileSize);

    u32 Offset = 0;
    while(Offset < FileSize)
    {
        instruction Decoded;
        Sim86_Decode8086Instruction(FileSize - Offset, s_Memory + Offset, &Decoded);
        if(Decoded.Op)
        {
			PrintInstruction(Decoded);
			if (ShouldExecute)
			{
				printf(" ; ");
				Offset = Simulate(Decoded, RegisterContents, s_Memory);
				PrintRegDiff(RegisterContents, OldRegContents);
			}
			printf("\n");
        }
        else
        {
            printf("Unrecognised instruction\n");
            break;
        }

		if (ShouldExecute)
		{
			memcpy(OldRegContents, RegisterContents, sizeof(RegisterContents));
		}
    }
		
	if (ShouldExecute)
	{
		PrintRegContents(RegisterContents);
	}
	if (ShouldDump)
	{
		DumpMemory(s_Memory, MEMORY_SIZE, "memory_dump.data");
	}


    return 0;
}