#include <stdio.h>
#include <stdlib.h>

#define ArrayCount(X) (sizeof(X) / sizeof((X)[0]))
#define Assert(X) {if (!(X)) __debugbreak();}

#include "sim86_shared.h"
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

int main(int ArgCount, char** Args)
{
    u32 Version = Sim86_GetVersion();
    if(Version != SIM86_VERSION)
    {
        printf("ERROR: Header file version doesn't match DLL.\n");
        return -1;
    }

	if (ArgCount != 2)
	{
		printf("Usage, homie\n");
		return 1;
	}

	u32 FileSize;
	u8* FileBuffer = LoadFile(Args[1], &FileSize);

    u32 Offset = 0;
    while(Offset < FileSize)
    {
        instruction Decoded;
        Sim86_Decode8086Instruction(FileSize - Offset, FileBuffer + Offset, &Decoded);
        if(Decoded.Op)
        {
            Offset += Decoded.Size;
			PrintInstruction(Decoded);
			printf("\n");
        }
        else
        {
            printf("Unrecognised instruction\n");
            break;
        }
    }
    
    return 0;
}