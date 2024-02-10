#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ArrayCount(X) (sizeof(X) / sizeof((X)[0]))
#define Assert(X) {if (!(X)) __debugbreak();}

struct segmented_access;

#include "sim86_shared.h"
#include "sim86_decode.h"
#include "decode_print.cpp"

u8* LoadFile(char* FileName, u32* OutFileSize)
{
	FILE* File = fopen(FileName, "rb");
	u8* FileBuffer;
	if (File)
	{
		u32 Start = ftell(File);
		fseek(File, 0, SEEK_END);
		*OutFileSize = (u32)(ftell(File)) - Start;
		fseek(File, 0, SEEK_SET);

		FileBuffer = (u8*)malloc(*OutFileSize);

		size_t BytesRead = fread(FileBuffer, 1, *OutFileSize, File);
		Assert(BytesRead == *OutFileSize);
		fclose(File);
	}
	else
	{
		printf("ERROR: Couldn't open file %s\n", FileName);
		Assert(false);
	}

	return FileBuffer;
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


void Perform8BitOp(u8* Dest, u8* Source, u16* OutFlags, operation_type Op)
{
	switch (Op)
	{
		case Op_mov:
		{
			*Dest = *Source;
		} break;
		case Op_add:
		{
			*Dest += *Source;
			SetFlags8(*Dest, OutFlags);
		} break;
		case Op_sub:
		{
			*Dest -= *Source;
			SetFlags8(*Dest, OutFlags);
		} break;
		case Op_cmp:
		{
			u8 Value = *Dest - *Source;
			SetFlags8(Value, OutFlags);
		} break;
	}
}

void Perform16BitOp(u16* Dest, u16* Source, u16* OutFlags, operation_type Op)
{
	switch (Op)
	{
		case Op_mov:
		{
			*Dest = *Source;
		} break;
		case Op_add:
		{
			*Dest += *Source;
			SetFlags16(*Dest, OutFlags);
		} break;
		case Op_sub:
		{
			*Dest -= *Source;
			SetFlags16(*Dest, OutFlags);
		} break;
		case Op_cmp:
		{
			u16 Value = *Dest - *Source;
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
		Perform16BitOp(&DestVal->Extended, &SourceVal->Extended, Flags, Op);
	}
	else
	{
		Assert(SourceReg.Offset <= 1);
		Assert(DestReg.Count < 2 && DestReg.Offset <= 1);

		if (SourceReg.Offset == 0)
		{
			if (DestReg.Offset == 0)
			{
				Perform8BitOp(&DestVal->Low, &SourceVal->Low, Flags, Op);
			}
			else
			{
				Perform8BitOp(&DestVal->High, &SourceVal->Low, Flags, Op);
			}
		}
		else
		{
			if (DestReg.Offset == 0)
			{
				Perform8BitOp(&DestVal->Low, &SourceVal->High, Flags, Op);
			}
			else
			{
				Perform8BitOp(&DestVal->High, &SourceVal->High, Flags, Op);
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
		Perform16BitOp(&DestVal->Extended, &((u16)Immediate.Value), Flags, Op);
	}
	else
	{
		Assert(DestReg.Offset <= 1);
		
		u8 ImmVal = (u8)Immediate.Value;
		if (DestReg.Offset == 0)
		{
			Perform8BitOp(&DestVal->Low, &ImmVal, Flags, Op);
		}
		else
		{
			Perform8BitOp(&DestVal->High, &ImmVal, Flags, Op);
		}
	}
}

u32 Simulate(instruction Instruction, reg_contents* RegContents)
{
	RegContents[Register_ip].Extended += Instruction.Size;
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
		if (Instruction.Op == Op_jne)
		{
			Assert(Instruction.Operands[0].Type == Operand_Immediate);

			if (!(RegContents[Register_flags].Extended & Flag_ZF))
			{
				s32 JumpOffset = Instruction.Operands[0].Immediate.Value;
				// This seems to work, but I'm slightly concerned about the signed/unsigned mixing...
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

	if (ArgCount > 3)
	{
		printf("Usage, homie\n");
		return 1;
	}

	char* FileName;
	b32 ShouldExecute = false;
	for (int i = 1; i < ArgCount; i++)
	{
		if (strcmp(ArgV[i], "-exec") == 0)
		{
			ShouldExecute = true;
		}
		else
		{
			FileName = ArgV[i];
		}
	}

	reg_contents RegisterContents[Register_count] = {0};
	reg_contents OldRegContents[Register_count] = {0};

	u32 FileSize;
	u8* FileBuffer = LoadFile(FileName, &FileSize);

    u32 Offset = 0;
    while(Offset < FileSize)
    {
        instruction Decoded;
        Sim86_Decode8086Instruction(FileSize - Offset, FileBuffer + Offset, &Decoded);
        if(Decoded.Op)
        {
			PrintInstruction(Decoded);
			if (ShouldExecute)
			{
				printf(" ; ");
				Offset = Simulate(Decoded, RegisterContents);
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

    return 0;
}