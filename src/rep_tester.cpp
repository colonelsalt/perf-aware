#pragma once

struct rep_test_results
{
	u64 NumTests;
	u64 TotalTime;
	u64 MaxTime;
	u64 MinTime;
};

enum class test_mode : u32
{
	Uninitialised,
	Testing,
	Completed,
	Error
};

struct rep_tester
{
	u64 TargetNumBytes;
	u64 CpuFreq;
	u64 TestAttemptTime; // How long to keep trying without finding a new min. time before finishing the test
	u64 TestStartTime;

	test_mode Mode;
	u32 NumOpenBlocks;
	u32 NumCloseBlocks;
	u64 ElapsedForCurrentTest; // Resets when new min. time found
	u64 ByteCount;

	rep_test_results Results;
};

static void BeginTest(rep_tester* Tester)
{
	Tester->NumOpenBlocks++;
	Tester->ElapsedForCurrentTest -= __rdtsc();
}

static void EndTest(rep_tester* Tester)
{
	Tester->NumCloseBlocks++;
	Tester->ElapsedForCurrentTest += __rdtsc();
}

static void AddBytes(rep_tester* Tester, u64 NumBytes)
{
	Tester->ByteCount += NumBytes;
}

static void Error(rep_tester* Tester, const char* Message)
{
	Tester->Mode = test_mode::Error;
	fprintf(stderr, "ERROR: %s\n", Message);
}

static void InitTestWave(rep_tester* Tester, u64 TargetNumBytes, u64 CpuFreq, u32 SecsPerAttempt)
{
	if (Tester->Mode == test_mode::Uninitialised)
	{
		Tester->Mode = test_mode::Testing;
		Tester->TargetNumBytes = TargetNumBytes;
		Tester->CpuFreq = CpuFreq;
		Tester->Results.MinTime = (u64)-1;
	}
	else if (Tester->Mode == test_mode::Completed)
	{
		Tester->Mode = test_mode::Testing;
		if (Tester->TargetNumBytes != TargetNumBytes)
		{
			Error(Tester, "TargetNumBytes changed");
		}
		if (Tester->CpuFreq != CpuFreq)
		{
			Error(Tester, "CPU frequency changed");
		}
	}

	Tester->TestAttemptTime = SecsPerAttempt * CpuFreq;
	Tester->TestStartTime = __rdtsc();
}

static void PrintTime(const char* Label, u64 TimeTsc, u64 CpuFreq, u64 NumBytes)
{
	printf("%s: %llu", Label, TimeTsc);
	if (CpuFreq)
	{
		f64 Secs = EstimateSecs(TimeTsc, CpuFreq);
		printf(" (%fms)", Secs * 1'000.0f);
		if (NumBytes)
		{
			constexpr f64 GIGABYTE = 1'024.0 * 1'024.0 * 1'024.0;
			f64 Throughput = NumBytes / (GIGABYTE * Secs);
			printf(" %fgb/s", Throughput);
		}
	}
}


// Invoked in between each test run to check whether to continue this test wave
static b32 IsStillTesting(rep_tester* Tester)
{
	if (Tester->Mode != test_mode::Testing)
	{
		return false;
	}

	u64 CurrentTime = __rdtsc();

	if (Tester->NumOpenBlocks)
	{
		if (Tester->NumOpenBlocks != Tester->NumCloseBlocks)
		{
			Error(Tester, "Unbalanced BeginTime/EndTime");
		}
		if (Tester->ByteCount != Tester->TargetNumBytes)
		{
			Error(Tester, "Byte count mismatch");
		}

		if (Tester->Mode == test_mode::Testing)
		{
			rep_test_results* Results = &Tester->Results;
		
			Results->NumTests++;
			Results->TotalTime += Tester->ElapsedForCurrentTest;

			if (Tester->ElapsedForCurrentTest > Results->MaxTime)
			{
				Results->MaxTime = Tester->ElapsedForCurrentTest;
			}
			if (Tester->ElapsedForCurrentTest < Results->MinTime)
			{
				Results->MinTime = Tester->ElapsedForCurrentTest;

				Tester->TestStartTime = CurrentTime;
				PrintTime("Min", Results->MinTime, Tester->CpuFreq, Tester->ByteCount);
				printf("               \r");
			}

			// Reset for next test run
			Tester->NumOpenBlocks = 0;
			Tester->NumCloseBlocks = 0;
			Tester->ElapsedForCurrentTest = 0;
			Tester->ByteCount = 0;
		}
	}

	if (CurrentTime - Tester->TestStartTime > Tester->TestAttemptTime)
	{
		Tester->Mode = test_mode::Completed;

		// Print results of test wave
		rep_test_results* Results = &Tester->Results;
		printf("                                                          \r");
		
		PrintTime("Min", Results->MinTime, Tester->CpuFreq, Tester->TargetNumBytes);
		printf("\n");

		PrintTime("Max", Results->MaxTime, Tester->CpuFreq, Tester->TargetNumBytes);
		printf("\n");

		if (Results->NumTests)
		{
			f64 AvgTime = (f64)Results->TotalTime / (f64)Results->NumTests;
			PrintTime("Avg", (u64)AvgTime, Tester->CpuFreq, Tester->TargetNumBytes);
			printf("\n");
		}
	}

	b32 Result = (Tester->Mode == test_mode::Testing);
	return Result;
}