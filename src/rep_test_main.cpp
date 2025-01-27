#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

#include "haver_util.h"
#include "rep_tester.cpp"
#include "read_file_test.cpp"

typedef void asm_func(u64 Count, u8 *Data);

extern "C" void Read_Arbitrary(u64 OuterLoopCount, u8 *Data, u64 InnerLoopCount);
#pragma comment (lib, "asm_nonp2_cache_test")

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

    constexpr u64 NumBigTests = 10;
    constexpr u64 NumSmallTests = 5;
    constexpr u64 NumTotalTests = NumBigTests * NumSmallTests;

    rep_tester Testers[NumTotalTests] = {};
    buffer Buffer = MallocBuff(GIGABYTE);
    u32 MinSizeP2 = 14;

    u64 RealSizes[NumTotalTests] = {};
    u64 BucketSizes[NumTotalTests] = {};

    for (u32 BigSizeIndex = 0; BigSizeIndex < NumBigTests; BigSizeIndex++)
    {
        u32 SizeP2 = MinSizeP2 + BigSizeIndex;
        u64 BigSize = 1ULL << SizeP2;
        u64 SmallSize = BigSize;
        for (u32 SmallSizeIndex = 0; SmallSizeIndex < NumSmallTests; SmallSizeIndex++)
        {
            u64 OuterLoopCount = GIGABYTE / SmallSize;
            u64 InnerLoopCount = SmallSize / 256;
            u64 RealSize = OuterLoopCount * SmallSize;

            u32 TotalIndex = NumSmallTests * BigSizeIndex + SmallSizeIndex;
            RealSizes[TotalIndex] = RealSize;
            BucketSizes[TotalIndex] = SmallSize;
            
            rep_tester* Tester = Testers + TotalIndex;
            //printf("\n--- %s ---\n", TestFunc.Name);
            printf("--- Reading %lluKB ---\n", SmallSize / 1'024);
            InitTestWave(Tester, RealSize, CpuFreq, 10);
            
            while (IsStillTesting(Tester))
            {
                BeginTest(Tester);
                //TestFunc.Function(Buffer.Size, Buffer.Memory);
                Read_Arbitrary(OuterLoopCount, Buffer.Memory, InnerLoopCount);
                EndTest(Tester);
                AddBytes(Tester, RealSize);
            }
            SmallSize += KILOBYTE;
        }
    }

    printf("Region sizes,gb/s\n");
    for (u32 i = 0; i < NumTotalTests; i++)
    {
        rep_tester* Tester = Testers + i;

        rep_values MinValue = Tester->Results.Min;
        f64 Secs = EstimateSecs((f64)MinValue.E[RepValue_CpuTimer], Tester->CpuFreq);
        f64 RealSize = (f64)RealSizes[i];
        f64 Bandwidth = MinValue.E[RepValue_NumBytes] / (RealSize * Secs);

        u64 Size = BucketSizes[i];
        printf("%llu,%f\n", Size, Bandwidth);
    }
	
}