#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

#include "haver_util.h"
#include "rep_tester.cpp"
#include "read_file_test.cpp"

typedef void asm_func(u64 Count, u8 *Data);

extern "C" void Read_x1(u64 Count, u8 *Data);
extern "C" void Read_x2(u64 Count, u8 *Data);
extern "C" void Read_x3(u64 Count, u8 *Data);
extern "C" void Read_x4(u64 Count, u8 *Data);
extern "C" void Write_x1(u64 Count, u8* Data);
extern "C" void Write_x2(u64 Count, u8* Data);
extern "C" void Write_x3(u64 Count, u8* Data);
extern "C" void Write_x4(u64 Count, u8* Data);
#pragma comment (lib, "asm_read_write_ports")

struct test_function
{
    char const* Name;
    asm_func* Function;
};
test_function TestFunctions[] =
{
    {"Read_x1", Read_x1},
    {"Read_x2", Read_x2},
    {"Read_x3", Read_x3},
    {"Read_x4", Read_x4},
    {"Write_x1", Write_x1},
    {"Write_x2", Write_x2},
    {"Write_x3", Write_x3},
    {"Write_x4", Write_x4},
};

buffer MallocBuff(u64 Size)
{
    buffer Result;
    Result.Memory = (u8*)malloc(Size);
    Result.Size = Size;
    return Result;
}

void FreeBuff(buffer* Buffer)
{
    if (Buffer->Memory)
    {
        free(Buffer->Memory);
    }
    *Buffer = {};
}


int main(int ArgC, char** ArgV)
{
    InitOsMetrics();
	u64 CpuFreq = EstimateCpuFreq(10);

#if 0
	if (ArgC != 2)
	{
		printf("Usage, homie.\n");
		return 1;
	}

	char* FileName = ArgV[1];

	struct __stat64 FileStat;
	_stat64(FileName, &FileStat);

	read_params Params = {};
	Params.DestBuffer.Size = FileStat.st_size;
	Params.DestBuffer.Memory = (u8*)malloc(FileStat.st_size);
	Params.FileName = FileName;

	if (!Params.DestBuffer.Memory)
	{
		printf("Failed to allocate buffer of size %llu for file %s\n", Params.DestBuffer.Size, FileName);
		return 1;
	}
#endif

    rep_tester Testers[ArrayCount(TestFunctions)] = {};
    buffer Buffer = MallocBuff(4 * KILOBYTE);
    u64 RepeatCount = GIGABYTE;

	while (true)
	{
        for (u32 i = 0; i < ArrayCount(TestFunctions); i++)
        {
            rep_tester* Tester = &Testers[i];
            test_function TestFunc = TestFunctions[i];

            printf("\n--- %s ---\n", TestFunc.Name);
            InitTestWave(Tester, Buffer.Size, CpuFreq, 10);
            
            while (IsStillTesting(Tester))
            {
                BeginTest(Tester);
                TestFunc.Function(Buffer.Size, Buffer.Memory);
                EndTest(Tester);
                AddBytes(Tester, Buffer.Size);
            }
        }
	}
}