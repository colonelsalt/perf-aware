#include "haver_util.h"

#include <stdio.h>
#include <stdlib.h>

f64* LoadBinaryAnswers(char* FileName, s64* OutNumElements)
{
	FILE* File = fopen(FileName, "rb");
	f64* Result = nullptr;
	if (File)
	{
		s64 Start = ftell(File);
		fseek(File, 0, SEEK_END);
		s64 FileSize = ftell(File) - Start;
		fseek(File, 0, SEEK_SET);

		Result = (f64*)malloc(FileSize);
		*OutNumElements = FileSize / sizeof(f64);

		size_t ElementsRead = fread(Result, sizeof(f64), *OutNumElements, File);
		Assert(ElementsRead == *OutNumElements);
		fclose(File);
	}
	else
	{
		printf("ERROR: Couldn't open file %s\n", FileName);
		Assert(false);
	}
	return Result;
}

struct cool_string
{
	u8* Data;
	s64 Length;

	u8 operator[](s64 Index)
	{
		Assert(Index >= 0 && Index < Length);
		return Data[Index];
	}
};

void CoolToC(cool_string String, char* OutBuffer)
{
	s64 i;
	for (i = 0; i < String.Length; i++)
	{
		OutBuffer[i] = String[i];
	}
	OutBuffer[i] = 0;
}

// Inclusive on both ends
cool_string Substring(cool_string String, s64 StartIndex, s64 EndIndex = -1)
{
	Assert(StartIndex >= 0 && StartIndex < String.Length);

	cool_string Result = {};
	Result.Data = String.Data + StartIndex;
	if (EndIndex == -1)
	{
		Result.Length = String.Length - StartIndex;
	}
	else
	{
		Assert(EndIndex > 0 && EndIndex < String.Length && StartIndex <= EndIndex);
		Result.Length = EndIndex - StartIndex + 1;
	}
	return Result;
}

s64 IndexOfChar(cool_string String, u8 Char)
{
	for (s64 i = 0; i < String.Length; i++)
	{
		if (String[i] == Char)
		{
			return i;
		}
	}
	return -1;
}

s64 LastIndexOfChar(cool_string String, u8 Char)
{
	for (s64 i = String.Length - 1; i >= 0; i--)
	{
		if (String[i] == Char)
		{
			return i;
		}
	}
	return -1;
}

s64 IndexOfCString(cool_string String, const char* CString)
{
	s64 Result = -1;
	s64 CIndex = 0;
	for (s64 i = 0; i < String.Length; i++)
	{
		if (String[i] == CString[CIndex])
		{
			if (Result == -1)
			{
				Result = i;
			}

			CIndex++;
			if (!CString[CIndex])
			{
				// Matched the whole substring
				break;
			}
		}
		else
		{
			Result = -1;
			CIndex = 0;
		}
	}
	return Result;
}

void PrintString(cool_string String)
{
	for (s64 i = 0; i < String.Length; i++)
	{
		printf("%c", String[i]);
	}
	printf("\n");
}

cool_string JsonFileToString(char* FileName)
{
	FILE* File = fopen(FileName, "rt");
	s64 Start = ftell(File);
	fseek(File, 0, SEEK_END);
	s64 Size = ftell(File) - Start;
	fseek(File, 0, SEEK_SET);

	u8* StringBuffer = (u8*)malloc(Size * sizeof(u8));
	if (!StringBuffer)
	{
		printf("Bad malloc, ooga booga\n");
		return {nullptr, -1};
	}

	// Note: Apparently BytesRead can turn out slightly smaller than Size, but that should be ok..?
	size_t BytesRead = fread(StringBuffer, sizeof(u8), Size, File);
	
	cool_string Result = {};
	Result.Data = StringBuffer;
	Result.Length = BytesRead;

	return Result;
}

struct perf_categories
{
	f64 Startup;
	f64 Read;
	f64 MiscSetup;
	f64 Parse;
	f64 Sum;
	f64 MiscOutput;
};

f64 Percentage(f64 Time, f64 TotalTime)
{
	f64 Result = (Time / TotalTime) * 100.0;
	return Result;
}

int main(int ArgC, char** ArgV)
{
	u64 ExecutionStart = __rdtsc();
	u64 CpuFreq = EstimateCpuFreq(10);

	perf_categories PerfCats = {};

	if (ArgC > 3)
	{
		printf("Usage, homie. Expecting input JSON file path, and optionally a reference binary answer file.\n");
		return 1;
	}

	PerfCats.Startup = EstimateMs(ExecutionStart, __rdtsc(), CpuFreq);

	f64* Answers = nullptr;
	if (ArgC == 3)
	{
		char* AnswerFileName = ArgV[2];
		s64 NumAnswers = -1;

		u64 BinaryReadStart = __rdtsc();

		Answers = LoadBinaryAnswers(AnswerFileName, &NumAnswers);

		u64 BinaryReadEnd = __rdtsc();
		PerfCats.Read += EstimateMs(BinaryReadStart, BinaryReadEnd, CpuFreq);
	}

	char* JsonFileName = ArgV[1];
	u64 JsonReadStart = __rdtsc();
	cool_string Json = JsonFileToString(JsonFileName);
	u64 JsonReadEnd = __rdtsc();
	PerfCats.Read += EstimateMs(JsonReadStart, JsonReadEnd, CpuFreq);

	u64 JsonParse1Start = __rdtsc();

	s64 OpenArrayIndex = IndexOfChar(Json, '[');
	cool_string CurrString = Substring(Json, OpenArrayIndex);
	
	s64 PairStartIndex = IndexOfChar(CurrString, '{');

	u64 JsonParse1End = __rdtsc();
	PerfCats.Parse += EstimateMs(JsonParse1Start, JsonParse1End, CpuFreq);

	f64 Sum = 0.0;
	s64 NumPairs = 0;
	f64 ErrorSum = 0.0;
	while (PairStartIndex != -1)
	{
		u64 JsonParse2Start = __rdtsc();

		CurrString = Substring(CurrString, PairStartIndex);

		CurrString = Substring(CurrString, IndexOfCString(CurrString, "x0"));
		cool_string X0String = Substring(CurrString, IndexOfChar(CurrString, ':') + 1, IndexOfChar(CurrString, ',') - 1);
		
		CurrString = Substring(CurrString, IndexOfCString(CurrString, "y0"));
		cool_string Y0String = Substring(CurrString, IndexOfChar(CurrString, ':') + 1, IndexOfChar(CurrString, ',') - 1);

		CurrString = Substring(CurrString, IndexOfCString(CurrString, "x1"));
		cool_string X1String = Substring(CurrString, IndexOfChar(CurrString, ':') + 1, IndexOfChar(CurrString, ',') - 1);

		CurrString = Substring(CurrString, IndexOfCString(CurrString, "y1"));
		cool_string Y1String = Substring(CurrString, IndexOfChar(CurrString, ':') + 1, IndexOfChar(CurrString, '}') - 1);

		char StrBuff[256];

		CoolToC(X0String, StrBuff);
		f64 X0 = atof(StrBuff);

		CoolToC(Y0String, StrBuff);
		f64 Y0 = atof(StrBuff);

		CoolToC(X1String, StrBuff);
		f64 X1 = atof(StrBuff);

		CoolToC(Y1String, StrBuff);
		f64 Y1 = atof(StrBuff);

		u64 JsonParse2End = __rdtsc();
		PerfCats.Parse += EstimateMs(JsonParse2Start, JsonParse2End, CpuFreq);

		u64 SumStart = __rdtsc();
		f64 Haversine = ReferenceHaversine(X0, Y0, X1, Y1);

		if (Answers)
		{
			f64 RealAnswer = Answers[NumPairs];
			ErrorSum += abs(Haversine - RealAnswer);
		}

		Sum += Haversine;
		u64 SumEnd = __rdtsc();
		PerfCats.Sum += EstimateMs(SumStart, SumEnd, CpuFreq);

		NumPairs++;

		u64 JsonParse3Start = __rdtsc();
		PairStartIndex = IndexOfChar(CurrString, '{');
		u64 JsonParse3End = __rdtsc();
		PerfCats.Parse += EstimateMs(JsonParse3Start, JsonParse3End, CpuFreq);
	}

	u64 OutputStart = __rdtsc();
	f64 Mean = Sum / NumPairs;

	printf("Number of pairs: %lld\n", NumPairs);
	printf("Average Haversine distance: %f\n", Mean);


	if (Answers)
	{
		printf("\nValidation:\n");
		printf("Reference sum: %f\n", Answers[NumPairs]);
		printf("Difference: %f\n", abs(Mean - Answers[NumPairs]));
		printf("Average per-pair difference: %f\n", ErrorSum / NumPairs);
	}

	u64 OutputEnd = __rdtsc();
	PerfCats.MiscOutput = EstimateMs(OutputStart, OutputEnd, CpuFreq);

	u64 ExecutionEnd = __rdtsc();

	f64 TotalTimeMs = (ExecutionEnd - ExecutionStart) / ((f64)CpuFreq) * 1'000;

	printf("\nPerformance metrics:\n");
	printf("Total time: %fms (CPU freq %llu)\n", TotalTimeMs, CpuFreq);
	printf(" Startup: %.0f (%.2f%%)\n", PerfCats.Startup, Percentage(PerfCats.Startup, TotalTimeMs));
	printf(" Read: %.0f (%.2f%%)\n", PerfCats.Read, Percentage(PerfCats.Read, TotalTimeMs));
	printf(" Parse: %.0f (%.2f%%)\n", PerfCats.Parse, Percentage(PerfCats.Parse, TotalTimeMs));
	printf(" Sum: %.0f (%.2f%%)\n", PerfCats.Sum, Percentage(PerfCats.Sum, TotalTimeMs));
	printf(" MiscOutput: %.0f (%.2f%%)\n", PerfCats.MiscOutput, Percentage(PerfCats.MiscOutput, TotalTimeMs));
}