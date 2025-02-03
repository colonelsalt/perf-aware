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

    StatValue_Seconds,
    StatValue_GbPerSec,
    StatValue_KbPerPageFault,

	RepValue_Count
};

struct rep_values
{
	u64 E[RepValue_Count];

    f64 PerCount[RepValue_Count];

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

struct rep_series_label
{
    char Chars[64];
};

struct rep_test_series
{
    buffer Buffer;

    u32 MaxNumRows;
    u32 NumCols;

    u32 RowIndex;
    u32 ColIndex;

    rep_test_results* TestResults; // [NumRows][NumCols]
    rep_series_label* RowLabels; // [NumRows]
    rep_series_label* ColLabels; // [NumCols]

    rep_series_label RowLabelLabel;
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

static b32 IsInBounds(rep_test_series* Series)
{
    b32 Result = (Series->ColIndex < Series->NumCols && Series->RowIndex < Series->MaxNumRows);
    return Result;
}

static void InitTestWave(rep_test_series* Series, rep_tester* Tester, u64 TargetNumBytes, u64 CpuFreq, u32 SecsPerAttempt)
{
    if (IsInBounds(Series))
    {
        printf("\n--- %s %s ---\n", Series->ColLabels[Series->ColIndex].Chars, Series->RowLabels[Series->RowIndex].Chars);
    }
    InitTestWave(Tester, TargetNumBytes, CpuFreq, SecsPerAttempt);
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

static rep_test_results* GetTestResults(rep_test_series* Series, u32 ColIndex, u32 RowIndex)
{
    rep_test_results* Result = nullptr;
    if (ColIndex < Series->NumCols && RowIndex < Series->MaxNumRows)
    {
        Result = Series->TestResults + (RowIndex * Series->NumCols + ColIndex);
    }
    return Result;
}

static void ComputeDerived(rep_values* Values, u64 CpuFreq)
{
    u64 NumTests = Values->E[RepValue_NumTests];
    f64 Divisor = NumTests ? (f64)NumTests : 1.0;

    f64* PerCount = Values->PerCount;
    for (u32 i = 0; i < ArrayCount(Values->PerCount); i++)
    {
        PerCount[i] = (f64)Values->E[i] / Divisor;
    }
    if (CpuFreq)
    {
        f64 Secs = EstimateSecs(PerCount[RepValue_CpuTimer], CpuFreq);
        PerCount[StatValue_Seconds] = Secs;

        if (PerCount[RepValue_NumBytes] > 0)
        {
            PerCount[StatValue_GbPerSec] = PerCount[RepValue_NumBytes] / (GIGABYTE * Secs);
        }
    }
    if (PerCount[RepValue_PageFaults] > 0)
    {
        PerCount[StatValue_KbPerPageFault] = PerCount[RepValue_NumBytes] / (PerCount[RepValue_PageFaults] * KILOBYTE);
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
                ComputeDerived(&Results->Min, Tester->CpuFreq);
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

static b32 IsStillTesting(rep_test_series* Series, rep_tester* Tester)
{
    b32 Result = IsStillTesting(Tester);

    if (!Result && IsInBounds(Series))
    {
        rep_test_results* TestResults = GetTestResults(Series, Series->ColIndex, Series->RowIndex);
        *TestResults = Tester->Results;

        Series->ColIndex++;
        if (Series->ColIndex >= Series->NumCols)
        {
            Series->ColIndex = 0;
            Series->RowIndex++;
        }
    }

    return Result;
}

static rep_test_series AllocTestSeries(u32 NumCols, u32 MaxNumRows)
{
    rep_test_series Result = {};

    u64 TestResultsSize = NumCols * MaxNumRows * sizeof(rep_test_results);
    u64 RowLabelsSize = MaxNumRows * sizeof(rep_series_label);
    u64 ColLabelsSize = NumCols * sizeof(rep_series_label);

    Result.Buffer = MallocBuff(TestResultsSize + RowLabelsSize + ColLabelsSize);

    Result.MaxNumRows = MaxNumRows;
    Result.NumCols = NumCols;

    Result.TestResults = (rep_test_results*)Result.Buffer.Memory;
    Result.RowLabels = (rep_series_label*)((u8*)Result.TestResults + TestResultsSize);
    Result.ColLabels = (rep_series_label*)((u8*)Result.RowLabels + RowLabelsSize);

    return Result;
}

static void SetRowLabelLabel(rep_test_series* Series, const char* Format, ...)
{
    rep_series_label* Label = &Series->RowLabelLabel;
    va_list Args;
    va_start(Args, Format);
    vsnprintf(Label->Chars, sizeof(Label->Chars), Format, Args);
    va_end(Args);
}

static void SetRowLabel(rep_test_series* Series, const char* Format, ...)
{
    if(IsInBounds(Series))
    {
        rep_series_label *Label = Series->RowLabels + Series->RowIndex;
        va_list Args;
        va_start(Args, Format);
        vsnprintf(Label->Chars, sizeof(Label->Chars), Format, Args);
        va_end(Args);
    }
}

static void SetColumnLabel(rep_test_series* Series, const char* Format, ...)
{
    if(IsInBounds(Series))
    {
        rep_series_label *Label = Series->ColLabels + Series->ColIndex;
        va_list Args;
        va_start(Args, Format);
        vsnprintf(Label->Chars, sizeof(Label->Chars), Format, Args);
        va_end(Args);
    }
}

static void PrintCsv(rep_test_series* Series, rep_value_type ValueType, f64 Coefficient = 1.0)
{
    printf("%s", Series->RowLabelLabel.Chars);
    for(u32 Col = 0; Col < Series->NumCols; Col++)
    {
        printf(",%s", Series->ColLabels[Col].Chars);
    }
    printf("\n");

    for(u32 Row = 0; Row < Series->RowIndex; Row++)
    {
        printf("%s", Series->RowLabels[Row].Chars);
        for(u32 Col = 0; Col < Series->NumCols; Col++)
        {
            rep_test_results* TestResults = GetTestResults(Series, Col, Row);
            printf(",%f", Coefficient*TestResults->Min.PerCount[ValueType]);
        }
        printf("\n");
    }
}