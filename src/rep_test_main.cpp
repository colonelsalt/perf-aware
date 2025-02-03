#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

#include "haver_util.h"
#include "rep_tester.cpp"
#include "read_file_test.cpp"

typedef void asm_func(u64 Count, u8 *Data);

extern "C" void Read_Aligned(u64 Count, u8 *Data);
extern "C" void Read_1BOff(u64 Count, u8 *Data);
extern "C" void Read_7BOff(u64 Count, u8 *Data);
extern "C" void Read_17BOff(u64 Count, u8 *Data);
extern "C" void Read_63BOff(u64 Count, u8 *Data);
#pragma comment (lib, "asm_unaligned_test")

struct test_function
{
    char const* Name;
    asm_func* Function;
};
test_function TestFunctions[] =
{
    {"Read_Aligned", Read_Aligned},
    {"Read_1BOff", Read_1BOff},
    {"Read_7BOff", Read_7BOff},
    {"Read_17BOff", Read_17BOff},
    {"Read_63BOff", Read_63BOff},
};


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
    buffer Buffer = MallocBuff(GIGABYTE);

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