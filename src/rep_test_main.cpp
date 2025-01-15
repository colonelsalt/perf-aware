#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

#include "haver_util.h"
#include "rep_tester.cpp"
#include "read_file_test.cpp"

struct test_function
{
	const char* Name;
	read_testing_func* Function;
};

test_function TestFunctions[] =
{
	{"WriteAllBytesC", WriteAllBytesC},
    {"WriteAllBytes", WriteAllBytes},
    {"NopAllBytes", NopAllBytes},
    {"CmpAllBytes", CmpAllBytes},
    {"DecAllBytes", DecAllBytes}
	//{"fread", FReadTest},
	//{"_read", ReadTest},
	//{"ReadFile", ReadFileTest}
};

int main(int ArgC, char** ArgV)
{
	if (ArgC != 2)
	{
		printf("Usage, homie.\n");
		return 1;
	}

	InitOsMetrics();
	u64 CpuFreq = EstimateCpuFreq(10);

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

	rep_tester Testers[ArrayCount(TestFunctions)][alloc_type::Count] = {};

	while (true)
	{
		for (u32 i = 0; i < ArrayCount(TestFunctions); i++)
		{
			for (u32 AllocType = alloc_type::None; AllocType < alloc_type::Count; AllocType++)
			{
				Params.AllocType = (alloc_type)AllocType;

				rep_tester* Tester = &Testers[i][AllocType];
				test_function TestFunc = TestFunctions[i];

				printf("\n--- %s%s%s ---\n",
				       TestFunc.Name,
				       AllocType ? " + " : "",
				       GetAllocName(Params.AllocType));
				InitTestWave(Tester, Params.DestBuffer.Size, CpuFreq, 10);
				TestFunc.Function(Tester, &Params);
			}
		}
	}
}