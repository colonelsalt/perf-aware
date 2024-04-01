#pragma once

enum class test_mode : u32
{
	Uninitialised,
	Testing,
	Completed,
	Error
};

enum rep_value_type
{
	RepValue_NumTests,
	RepValue_CpuTimer,
	RepValue_PageFaults,
	RepValue_NumBytes,

	RepValue_Count
};

struct rep_values
{
	u64 E[RepValue_Count];

	u64& operator[](u64 Index)
	{
		Assert(Index >= 0 && Index < RepValue_Count);

		u64& Result = E[Index];
		return Result;
	}
};

struct rep_test_results
{
	rep_values Total;
	rep_values Max;
	rep_values Min;
};

struct rep_tester
{
	u64 TargetNumBytes;
	u64 CpuFreq;
	u64 TestAttemptTime; // How long to keep trying without finding a new min. time before finishing the test run
	u64 TestStartTime;

	test_mode Mode;
	u32 NumOpenBlocks;
	u32 NumCloseBlocks;

	rep_values AccValues; // Accumulated values for all runs of this test instance
	rep_test_results Results;
};

static void BeginTest(rep_tester* Tester)
{
	Tester->NumOpenBlocks++;

	Tester->AccValues[RepValue_CpuTimer] -= __rdtsc();
	Tester->AccValues[RepValue_PageFaults] -= ReadNumOsPageFaults();
}

static void EndTest(rep_tester* Tester)
{
	Tester->NumCloseBlocks++;

	Tester->AccValues[RepValue_CpuTimer] += __rdtsc();
	Tester->AccValues[RepValue_PageFaults] += ReadNumOsPageFaults();
}

static void AddBytes(rep_tester* Tester, u64 NumBytes)
{
	Tester->AccValues[RepValue_NumBytes] += NumBytes;
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
		Tester->Results.Min[RepValue_CpuTimer] = (u64)-1;
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

static void PrintRepValues(const char* Label, rep_values Values, u64 CpuFreq)
{
	// NOTE: Min. and max. values will always have TestCount of 1 - only "total/avg" values will have more
	f64 Divisor;
	if (Values[RepValue_NumTests] == 0)
	{
		Divisor = 1.0;
	}
	else
	{
		Divisor = (f64)Values[RepValue_NumTests];
	}

	f64 FloatValues[RepValue_Count];
	for (u64 i = 0; i < RepValue_Count; i++)
	{
		FloatValues[i] = (f64)Values[i] / Divisor;
	}

	printf("%s: %.0f", Label, FloatValues[RepValue_CpuTimer]);

	if (CpuFreq)
	{
		f64 Secs = EstimateSecs(FloatValues[RepValue_CpuTimer], CpuFreq);
		printf(" (%fms)", Secs * 1'000.0f);
		if (FloatValues[RepValue_NumBytes] > 0.0)
		{
			constexpr f64 GIGABYTE = 1'024.0 * 1'024.0 * 1'024.0;
			f64 Throughput = FloatValues[RepValue_NumBytes] / (GIGABYTE * Secs);
			printf(" %fgb/s", Throughput);
		}
	}

	if (FloatValues[RepValue_PageFaults] > 0.0)
	{
		f64 NumPageFaults = FloatValues[RepValue_PageFaults];
		printf(" PF: %0.4f (%0.4fkb/fault)", NumPageFaults, FloatValues[RepValue_NumBytes] / (NumPageFaults * 1'024.0));
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
		if (Tester->AccValues[RepValue_NumBytes] != Tester->TargetNumBytes)
		{
			Error(Tester, "Byte count mismatch");
		}

		if (Tester->Mode == test_mode::Testing)
		{
			rep_test_results* Results = &Tester->Results;
		
			Tester->AccValues[RepValue_NumTests] = 1;
			for (u64 i = 0; i < RepValue_Count; i++)
			{
				Results->Total[i] += Tester->AccValues[i];
			}

			if (Tester->AccValues[RepValue_CpuTimer] > Results->Max[RepValue_CpuTimer])
			{
				Results->Max = Tester->AccValues;
			}
			if (Tester->AccValues[RepValue_CpuTimer] < Results->Min[RepValue_CpuTimer])
			{
				Results->Min = Tester->AccValues;

				Tester->TestStartTime = CurrentTime;
				PrintRepValues("Min", Results->Min, Tester->CpuFreq);
				printf("                                   \r");
			}

			// Reset for next test run
			Tester->NumOpenBlocks = 0;
			Tester->NumCloseBlocks = 0;
			Tester->AccValues = {};
		}
	}

	if (CurrentTime - Tester->TestStartTime > Tester->TestAttemptTime)
	{
		Tester->Mode = test_mode::Completed;

		// Print results of test wave
		rep_test_results* Results = &Tester->Results;
		printf("                                                          \r");
		
		PrintRepValues("Min", Results->Min, Tester->CpuFreq);
		printf("\n");

		PrintRepValues("Max", Results->Max, Tester->CpuFreq);
		printf("\n");

		PrintRepValues("Avg", Results->Total, Tester->CpuFreq);
		printf("\n");
	}

	b32 Result = (Tester->Mode == test_mode::Testing);
	return Result;
}