#pragma once

#include <windows.h>
#include <fcntl.h>
#include <io.h>

enum alloc_type : u32
{
	None,
	Malloc,

	Count
};

struct read_params
{
	alloc_type AllocType;
	buffer DestBuffer;
	const char* FileName;
};

typedef void read_testing_func(rep_tester* Tester, read_params* Params);

static const char* GetAllocName(alloc_type Type)
{
	switch (Type)
	{
		case alloc_type::None:
			return "";
		case alloc_type::Malloc:
			return "Malloc";
		default:
			return "ERROR-TYPE";
	}
}

static void Allocate(read_params* Params, buffer* OutBuffer)
{
	switch (Params->AllocType)
	{
		case alloc_type::None:
			break;
		
		case alloc_type::Malloc:
		{
			OutBuffer->Size = Params->DestBuffer.Size;
			OutBuffer->Memory = (u8*)malloc(Params->DestBuffer.Size);
		} break;

		default:
		{
			fprintf(stderr, "ERROR: Unrecognised allocation type\n");
		} break;
	}
}

static void Deallocate(read_params* Params, buffer* OutBuffer)
{
	switch (Params->AllocType)
	{
		case alloc_type::None:
			break;

		case alloc_type::Malloc:
		{
			free(OutBuffer->Memory);
			*OutBuffer = {};
		} break;

		default:
		{
			fprintf(stderr, "ERROR: Unrecognised allocation type\n");
		} break;
	}
}

void WriteAllBytesC(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		buffer DestBuffer = Params->DestBuffer;
		Allocate(Params, &DestBuffer);

		BeginTest(Tester);
		for (u64 i = 0; i < DestBuffer.Size; i++)
		{
			DestBuffer.Memory[i] = (u8)i;
		}
		EndTest(Tester);

		AddBytes(Tester, DestBuffer.Size);
		Deallocate(Params, &DestBuffer);
	}
}

extern "C" void MOVAllBytesASM(u64 Count, u8* Data);
extern "C" void NOPAllBytesASM(u64 Count);
extern "C" void CMPAllBytesASM(u64 Count);
extern "C" void DECAllBytesASM(u64 Count);
#pragma comment (lib, "asm_loop")

void WriteAllBytes(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		buffer DestBuffer = Params->DestBuffer;
		Allocate(Params, &DestBuffer);

		BeginTest(Tester);
        MOVAllBytesASM(DestBuffer.Size, DestBuffer.Memory);
		EndTest(Tester);

		AddBytes(Tester, DestBuffer.Size);
		Deallocate(Params, &DestBuffer);
	}
}

void NopAllBytes(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		buffer DestBuffer = Params->DestBuffer;
		Allocate(Params, &DestBuffer);

		BeginTest(Tester);
        NOPAllBytesASM(DestBuffer.Size);
		EndTest(Tester);

		AddBytes(Tester, DestBuffer.Size);
		Deallocate(Params, &DestBuffer);
	}
}

void CmpAllBytes(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		buffer DestBuffer = Params->DestBuffer;
		Allocate(Params, &DestBuffer);

		BeginTest(Tester);
        CMPAllBytesASM(DestBuffer.Size);
		EndTest(Tester);

		AddBytes(Tester, DestBuffer.Size);
		Deallocate(Params, &DestBuffer);
	}
}

void DecAllBytes(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		buffer DestBuffer = Params->DestBuffer;
		Allocate(Params, &DestBuffer);

		BeginTest(Tester);
        DECAllBytesASM(DestBuffer.Size);
		EndTest(Tester);

		AddBytes(Tester, DestBuffer.Size);
		Deallocate(Params, &DestBuffer);
	}
}


void FReadTest(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		FILE* File = fopen(Params->FileName, "rb");
		if (File)
		{
			buffer DestBuffer = Params->DestBuffer;
			Allocate(Params, &DestBuffer);

			BeginTest(Tester);
			size_t Result = fread(DestBuffer.Memory, DestBuffer.Size, 1, File);
			EndTest(Tester);
			if (Result == 1)
			{
				AddBytes(Tester, DestBuffer.Size);
			}
			else
			{
				Error(Tester, "fread failed");
			}

			Deallocate(Params, &DestBuffer);
			fclose(File);
		}
		else
		{
			Error(Tester, "fopen failed");
		}
	}
}

void ReadTest(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		int File = _open(Params->FileName, _O_BINARY | _O_RDONLY);
		if (File == -1)
		{
			Error(Tester, "_open failed");
		}
		else
		{
			buffer DestBuffer = Params->DestBuffer;
			Allocate(Params, &DestBuffer);

			u64 BytesRemaining = DestBuffer.Size;
			u8* Dest = DestBuffer.Memory;
			while (BytesRemaining)
			{
				u32 ReadSize = INT_MAX;
				if ((u64)ReadSize > BytesRemaining)
				{
					ReadSize = (u32)BytesRemaining;
				}

				BeginTest(Tester);
				int Result = _read(File, Dest, ReadSize);
				EndTest(Tester);

				if (Result == (int)ReadSize)
				{
					AddBytes(Tester, ReadSize);
				}
				else
				{
					Error(Tester, "_read failed");
					break;
				}

				BytesRemaining -= ReadSize;
				Dest += ReadSize;
			}

			Deallocate(Params, &DestBuffer);
			_close(File);
		}
	}
}

void ReadFileTest(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		HANDLE File = CreateFileA(Params->FileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (File == INVALID_HANDLE_VALUE)
		{
			Error(Tester, "CreateFileA failed");
		}
		else
		{
			buffer DestBuffer = Params->DestBuffer;
			Allocate(Params, &DestBuffer);

			u64 BytesRemaining = DestBuffer.Size;
			u8* Dest = DestBuffer.Memory;
			while (BytesRemaining)
			{
				u32 ReadSize = (u32)-1;
				if ((u64)ReadSize > BytesRemaining)
				{
					ReadSize = (u32)BytesRemaining;
				}

				DWORD BytesRead = 0;
				BeginTest(Tester);
				BOOL Result = ReadFile(File, Dest, ReadSize, &BytesRead, 0);
				EndTest(Tester);

				if (Result && BytesRead == ReadSize)
				{
					AddBytes(Tester, ReadSize);
				}
				else
				{
					Error(Tester, "ReadFile failed");
					break;
				}
				
				BytesRemaining -= ReadSize;
				Dest += ReadSize;
			}

			Deallocate(Params, &DestBuffer);
			CloseHandle(File);
		}
	}
}