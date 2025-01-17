#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

#include "haver_util.h"
#include "rep_tester.cpp"
#include "read_file_test.cpp"

typedef void asm_func(u64 Count, u8 *Data);

extern "C" void NOP3x1AllBytes(u64 Count, u8 *Data);
extern "C" void NOP1x3AllBytes(u64 Count, u8 *Data);
extern "C" void NOP1x9AllBytes(u64 Count, u8 *Data);
#pragma comment (lib, "asm_nops")

enum branching_pattern
{
    BPattern_NeverTaken,
    BPattern_AlwaysTaken,
    BPattern_Every2,
    BPattern_Every3,
    BPattern_Every4,
    BPattern_CrtRandom,
    BPattern_OsRandom,

    BPattern_Count
};

extern "C" void ConditionalNOP(u64 Count, u8 *Data);
#pragma comment (lib, "asm_conditional_nop")

struct test_function
{
    char const* Name;
    asm_func* Function;
};
test_function TestFunctions[] =
{
    {"Conditional NOP", ConditionalNOP},
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

static const char* FillWithPattern(branching_pattern Pattern, buffer Buffer)
{
    const char* PatternName = "UNKNOWN";

    if (Pattern == BPattern_OsRandom)
    {
        PatternName = "OS random";
        FillRandom(Buffer);
    }

    else
    {
        for (u64 i = 0; i < Buffer.Size; i++)
        {
            u8 Value = 0;
            switch (Pattern)
            {
                case BPattern_NeverTaken:
                {
                    PatternName = "Never taken";
                    Value = 0;
                } break;

                case BPattern_AlwaysTaken:
                {
                    PatternName = "Always taken";
                    Value = 1;
                } break;

                case BPattern_Every2:
                {
                    PatternName = "Every 2";
                    Value = (i % 2 == 0);
                } break;

                case BPattern_Every3:
                {
                    PatternName = "Every 3";
                    Value = (i % 3 == 0);
                } break;

                case BPattern_Every4:
                {
                    PatternName = "Every 4";
                    Value = (i % 4 == 0);
                } break;

                case BPattern_CrtRandom:
                {
                    PatternName = "CRT random";
                    Value = (u8)rand();
                } break;

                default:
                {
                    fprintf(stderr, "Invalid branching pattern\n");
                } break;
            }

            Buffer.Memory[i] = Value;
        }
    }

    return PatternName;
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

    rep_tester Testers[BPattern_Count][ArrayCount(TestFunctions)] = {};
    buffer Buffer = MallocBuff(GIGABYTE);

	while (true)
	{
        for (u32 i = 0; i < BPattern_Count; i++)
        {
            branching_pattern Pattern = (branching_pattern)i;
            const char* PatternName = FillWithPattern(Pattern, Buffer);

            for (u32 j = 0; j < ArrayCount(TestFunctions); j++)
            {
                rep_tester* Tester = &Testers[i][j];
                test_function TestFunc = TestFunctions[j];

                printf("\n--- %s, %s ---\n", TestFunc.Name, PatternName);
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
}