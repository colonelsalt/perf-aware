#pragma once

#include <windows.h>
#include <fcntl.h>
#include <io.h>

struct read_params
{
	buffer DestBuffer;
	const char* FileName;
};

typedef void read_testing_func(rep_tester* Tester, read_params* Params);

void FReadTest(rep_tester* Tester, read_params* Params)
{
	while (IsStillTesting(Tester))
	{
		FILE* File = fopen(Params->FileName, "rb");
		if (File)
		{
			buffer DestBuffer = Params->DestBuffer;

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

			CloseHandle(File);
		}
	}
}