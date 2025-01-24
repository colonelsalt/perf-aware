#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

#include "haver_util.h"
#include "rep_tester.cpp"
#include "read_file_test.cpp"

typedef void asm_func(u64 Count, u8 *Data);

extern "C" void Read_256(u64 Count, u8 *Data, u64 Mask);
#pragma comment (lib, "asm_cache_bandwidth_tests")

struct test_function
{
    char const* Name;
    asm_func* Function;
};

#if 0
test_function TestFunctions[] =
{
    {"Read_48K", Read_48K},
    {"Read_1M", Read_1M},
    {"Read_16M", Read_16M},
    {"Read_32M", Read_32M},
    {"Read_64M", Read_64M},
    {"Read_1G", Read_1G},
};
#endif

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

    rep_tester Testers[30] = {};
    buffer Buffer = MallocBuff(GIGABYTE);
    u32 MinSizeP2 = 10;

    for (u32 SizeP2 = MinSizeP2; SizeP2 < ArrayCount(Testers); SizeP2++)
    {
        rep_tester* Tester = Testers + SizeP2;

        u64 Size = 1ULL << SizeP2;
        u64 Mask = Size - 1;

        //printf("\n--- %s ---\n", TestFunc.Name);
        printf("--- Read256 of %lluKB ---\n", Size / 1'024);
        InitTestWave(Tester, Buffer.Size, CpuFreq, 10);
            
        while (IsStillTesting(Tester))
        {
            BeginTest(Tester);
            //TestFunc.Function(Buffer.Size, Buffer.Memory);
            Read_256(Buffer.Size, Buffer.Memory, Mask);
            EndTest(Tester);
            AddBytes(Tester, Buffer.Size);
        }
    }

    printf("Region sizes,gb/s\n");
    for (u32 SizeP2 = MinSizeP2; SizeP2 < ArrayCount(Testers); SizeP2++)
    {
        rep_tester* Tester = Testers + SizeP2;

        rep_values MinValue = Tester->Results.Min;
        f64 Secs = EstimateSecs((f64)MinValue.E[RepValue_CpuTimer], Tester->CpuFreq);
        f64 Bandwidth = MinValue.E[RepValue_NumBytes] / ((f64)GIGABYTE * Secs);

        u64 Size = 1ULL << SizeP2;
        printf("%llu,%f\n", Size, Bandwidth);
    }
	
}