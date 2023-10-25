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

union reg_contents
{
	struct
	{
		u8 Low;
		u8 High;
	};
	u16 Extended;
};

void PrintRegContents(reg_contents* Regs)
{
	printf("\nFinal registers: \n");
	for (u32 i = Register_a; i < Register_count; ++i)
	{
		register_access Reg = {};
		Reg.Index = i;
		Reg.Count = 2;

		printf("\t%s: 0x%04x (%d)\n", GetRegName(Reg), Regs[i].Extended, Regs[i].Extended);
	}
}

void PrintRegDiff(reg_contents* RegsCurrent, reg_contents* RegsOld)
{
	for (u32 i = Register_a; i < Register_count; i++)
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
}

void Simulate(instruction Instruction, reg_contents* RegContents)
{
	if (Instruction.Op == Op_mov)
	{
		if (Instruction.Operands[0].Type == Operand_Register)
		{
			register_access DestReg = Instruction.Operands[0].Register;
			Assert(DestReg.Index > Register_none && DestReg.Index < Register_count);
			if (Instruction.Operands[1].Type == Operand_Immediate)
			{
				immediate Immediate = Instruction.Operands[1].Immediate;
				if (DestReg.Count == 2)
				{
					RegContents[DestReg.Index].Extended = (u16)Immediate.Value;
				}
				else
				{
					Assert(DestReg.Offset <= 1);
					if (DestReg.Offset == 0)
					{
						RegContents[DestReg.Index].Low = (u8)Immediate.Value;
					}
					else
					{
						RegContents[DestReg.Index].High = (u8)Immediate.Value;
					}
				}
			}
			else if (Instruction.Operands[1].Type == Operand_Register)
			{
				register_access SourceReg = Instruction.Operands[1].Register;
				Assert(SourceReg.Index > Register_none && SourceReg.Index < Register_count);

				if (SourceReg.Count == 2)
				{
					RegContents[DestReg.Index].Extended = RegContents[SourceReg.Index].Extended;
				}
				else
				{
					Assert(SourceReg.Offset <= 1);
					Assert(DestReg.Count < 2 && DestReg.Offset <= 1);

					if (SourceReg.Offset == 0)
					{
						if (DestReg.Offset == 0)
						{
							RegContents[DestReg.Index].Low = RegContents[SourceReg.Index].Low;
						}
						else
						{
							RegContents[DestReg.Index].High = RegContents[SourceReg.Index].Low;
						}
					}
					else
					{
						if (DestReg.Offset == 0)
						{
							RegContents[DestReg.Index].Low = RegContents[SourceReg.Index].High;
						}
						else
						{
							RegContents[DestReg.Index].High = RegContents[SourceReg.Index].High;
						}
					}
				}

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
		// TODO...
	}
}


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
            Offset += Decoded.Size;
			PrintInstruction(Decoded);
			if (ShouldExecute)
			{
				printf(" ; ");
				Simulate(Decoded, RegisterContents);
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