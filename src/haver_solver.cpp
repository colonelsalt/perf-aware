#include <stdio.h>
#include "haver_util.h"

#include "profiler.h"
#include "rep_tester.cpp"
#include <stdlib.h>





struct test_func
{
    const char* Name;
    haver_compute_func* Compute;
    haver_verify_func* Verify;
};

static test_func TestFunctions[] =
{
    {"ReferenceHaversine", RefSumHaversine, RefVerifyHaversine}
};




f64 SumHaverPairs(haver_pair* Pairs, s64 NumPairs)
{
	TimeBandwidth(__func__, NumPairs * sizeof(haver_pair));

	f64 Sum = 0.0;
	for (s64 i = 0; i < NumPairs; i++)
	{
		haver_pair* Pair = Pairs + i;
		f64 Haversine = ReferenceHaversine(Pair->X0, Pair->Y0, Pair->X1, Pair->Y1);
		Sum += Haversine;
	}
	f64 Mean = Sum / NumPairs;
	return Mean;
}

int main(int ArgC, char** ArgV)
{
	//BeginProfile();

	if (ArgC != 3)
	{
		printf("Usage, homie. Expecting input JSON file path, and a reference binary answer file.\n");
		return 1;
	}

    InitOsMetrics();
	u64 CpuFreq = EstimateCpuFreq(10);

    haver_setup Setup = SetUpHaversine(ArgV[1], ArgV[2]);
    rep_test_series TestSeries = AllocTestSeries(ArrayCount(TestFunctions), 1);

    SetRowLabelLabel(&TestSeries, "Test");
    SetRowLabel(&TestSeries, "Haversine");

    InitMathsDomains();
    for (u32 i = 0; i < ArrayCount(TestFunctions); i++)
    {
        test_func Function = TestFunctions[i];
        SetColumnLabel(&TestSeries, "%s", Function.Name);

        rep_tester Tester = {};
        InitTestWave(&TestSeries, &Tester, Setup.ParsedByteSize, CpuFreq, 10);

        u64 ErrorCount = Function.Verify(Setup);
        u64 SumsErrorCount = 0;

        while (IsStillTesting(&TestSeries, &Tester))
        {
            BeginTest(&Tester);
            f64 Check = Function.Compute(Setup);
            AddBytes(&Tester, Setup.ParsedByteSize);
            EndTest(&Tester);

            if (!ApproxEqual(Check, Setup.AnswerSum))
            {
                SumsErrorCount++;
            }

            if (SumsErrorCount || ErrorCount)
            {
                fprintf(stderr, "WARNING: %llu haversines mismatched (%llu individual sum mismatches)\n", ErrorCount, SumsErrorCount);
            }
        }

        PrintCsv(&TestSeries, StatValue_GbPerSec);
        PrintMathsDomains();
        printf("\nDONE\n");
    }

#if 0
	f64* Answers = nullptr;
	if (ArgC == 3)
	{
		char* AnswerFileName = ArgV[2];
		s64 NumAnswers = -1;

		Answers = LoadBinaryAnswers(AnswerFileName, &NumAnswers);
	}

	char* JsonFileName = ArgV[1];
	cool_string Json = JsonFileToString(JsonFileName);

	s64 NumPairs;
	haver_pair* HaverPairs = ParseHaverPairs(Json, &NumPairs);

	f64 Mean = SumHaverPairs(HaverPairs, NumPairs);

	printf("Number of pairs: %lld\n", NumPairs);
	printf("Average Haversine distance: %f\n", Mean);


	if (Answers)
	{
		printf("\nValidation:\n");
		printf("Reference sum: %f\n", Answers[NumPairs]);
		printf("Difference: %f\n", abs(Mean - Answers[NumPairs]));
	}

	EndAndPrintProfile();
#endif
}