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
	{"fread", FReadTest},
	{"_read", ReadTest},
	{"ReadFile", ReadFileTest}
};

int main(int ArgC, char** ArgV)
{
	if (ArgC != 2)
	{
		printf("Usage, homie.\n");
		return 1;
	}

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

	rep_tester Testers[ArrayCount(TestFunctions)] = {};

	for (u32 i = 0; i < ArrayCount(TestFunctions); i++)
	{
		rep_tester* Tester = Testers + i;
		test_function TestFunc = TestFunctions[i];

		printf("\n--- %s ---\n", TestFunc.Name);
		InitTestWave(Tester, Params.DestBuffer.Size, CpuFreq, 30);
		TestFunc.Function(Tester, &Params);
	}

}