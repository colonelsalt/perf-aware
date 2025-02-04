#pragma once

#include <cstdint>
#include <cmath>

#define ArrayCount(X) (sizeof(X) / sizeof((X)[0]))
#define Assert(X) {if (!(X)) __debugbreak();}

typedef int8_t s8;
typedef uint8_t u8;
typedef float f32;
typedef double f64;
typedef uint32_t u32;
typedef int32_t s32;
typedef int32_t b32;
typedef int64_t s64;
typedef uint64_t u64;

static constexpr u64 KILOBYTE = 1'024;
static constexpr u64 MEGABYTE = 1'024 * KILOBYTE;
static constexpr u64 GIGABYTE = 1'024 * MEGABYTE;


static constexpr f64 EARTH_RADIUS = 6'372.8;

struct buffer
{
	u8* Memory;
	u64 Size;
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
	//TimeFunction;

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

struct haver_pair
{
	f64 X0;
	f64 Y0;
	f64 X1;
	f64 Y1;
};

struct haver_setup
{
    cool_string JsonString;

    u64 ParsedByteSize;

    u64 NumPairs;
    haver_pair* Pairs;
    
    f64* Answers;
    u64 NumAnswers;

    f64 AnswerSum;

    b32 IsValid;
};

f64* LoadBinaryAnswers(char* FileName, u64* OutNumElements)
{
	//TimeFunction;

	FILE* File = fopen(FileName, "rb");
	f64* Result = nullptr;
	if (File)
	{
		s64 Start = ftell(File);
		fseek(File, 0, SEEK_END);
		s64 FileSize = ftell(File) - Start;
		fseek(File, 0, SEEK_SET);

		Result = (f64*)malloc(FileSize);
        s64 NumElems = FileSize / sizeof(f64);
        if (NumElems < 0)
        {
            fprintf(stderr, "ERROR: Negative size bro\n");
            Assert(false);
        }

		*OutNumElements = (u64)NumElems;
		{
			//TimeBandwidth("fread binary", FileSize);
			size_t ElementsRead = fread(Result, sizeof(f64), *OutNumElements, File);
			Assert(ElementsRead == *OutNumElements);
			fclose(File);
		}
	}
	else
	{
		printf("ERROR: Couldn't open file %s\n", FileName);
		Assert(false);
	}
	return Result;
}

cool_string JsonFileToString(char* FileName)
{
	//TimeFunction;

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

	cool_string Result = {};
	{
		//TimeBandwidth("fread text", Size);
		// Note: Apparently BytesRead can turn out slightly smaller than Size, but that should be ok..?
		size_t BytesRead = fread(StringBuffer, sizeof(u8), Size, File);
	
		Result.Data = StringBuffer;
		Result.Length = BytesRead;
	}
	return Result;
}

haver_pair* ParseHaverPairs(cool_string Json, u64* OutNumPairs)
{
	//TimeBandwidth(__func__, Json.Length);

	*OutNumPairs = 0;

	u64 MinBufferSize = Json.Length / 6;
	haver_pair* Result = (haver_pair*)malloc(MinBufferSize * sizeof(haver_pair));

	s64 OpenArrayIndex = IndexOfChar(Json, '[');
	cool_string CurrString = Substring(Json, OpenArrayIndex);
	
	s64 PairStartIndex = IndexOfChar(CurrString, '{');

	while (PairStartIndex != -1)
	{
		haver_pair* Pair = Result + *OutNumPairs;

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
		Pair->X0 = atof(StrBuff);

		CoolToC(Y0String, StrBuff);
		Pair->Y0 = atof(StrBuff);

		CoolToC(X1String, StrBuff);
		Pair->X1 = atof(StrBuff);

		CoolToC(Y1String, StrBuff);
		Pair->Y1 = atof(StrBuff);

		(*OutNumPairs)++;

		PairStartIndex = IndexOfChar(CurrString, '{');
	}
	return Result;
}

static haver_setup SetUpHaversine(char* JsonFileName, char* AnswerFileName)
{
    haver_setup Result = {};

    Result.JsonString = JsonFileToString(JsonFileName);
    Result.Answers = LoadBinaryAnswers(AnswerFileName, &Result.NumAnswers);

    constexpr u32 MinJsonPairEncoding = 16;
    u64 MaxNumPairs = Result.JsonString.Length / MinJsonPairEncoding;

    Result.Pairs = ParseHaverPairs(Result.JsonString, &Result.NumPairs);
    if (Result.NumAnswers == Result.NumPairs + 1)
    {
        Result.AnswerSum = Result.Answers[Result.NumPairs];
        Result.ParsedByteSize = sizeof(haver_pair) * Result.NumPairs;

        printf("Source JSON: %lluMB\n", Result.JsonString.Length / MEGABYTE);
        printf("Parsed: %lluMB (%llu pairs)\n", Result.ParsedByteSize / MEGABYTE, Result.NumPairs);

        Result.IsValid = Result.NumPairs != 0;
    }
    else
    {
        fprintf(stderr, "ERROR: JSON source data has %llu pairs, but answer file has %llu values (should be %llu).\n",
                Result.NumPairs, Result.NumAnswers, Result.NumPairs + 1);
    }

    return Result;
}

typedef f64 haver_compute_func(haver_setup Setup);
typedef u64 haver_verify_func(haver_setup Setup);

static f64 Square(f64 A)
{
    f64 Result = (A*A);
    return Result;
}

static f64 RadiansFromDegrees(f64 Degrees)
{
    f64 Result = 0.01745329251994329577f * Degrees;
    return Result;
}

enum maths_func
{
    MathsFunc_Sin,
    MathsFunc_Cos,
    MathsFunc_ArcSin,
    MathsFunc_Sqrt,

    MathsFunc_Count
};

struct maths_func_domain
{
    f64 Min[MathsFunc_Count];
    f64 Max[MathsFunc_Count];
};

static maths_func_domain s_Domains;

static f64 Sin(f64 Angle)
{
    if (Angle < s_Domains.Min[MathsFunc_Sin])
    {
        s_Domains.Min[MathsFunc_Sin] = Angle;
    }
    if (Angle > s_Domains.Max[MathsFunc_Sin])
    {
        s_Domains.Max[MathsFunc_Sin] = Angle;
    }

    f64 Result = sin(Angle);
    return Result;
}

static f64 Cos(f64 Angle)
{
    if (Angle < s_Domains.Min[MathsFunc_Cos])
    {
        s_Domains.Min[MathsFunc_Cos] = Angle;
    }
    if (Angle > s_Domains.Max[MathsFunc_Cos])
    {
        s_Domains.Max[MathsFunc_Cos] = Angle;
    }

    f64 Result = cos(Angle);
    return Result;
}

static f64 ArcSin(f64 Sine)
{
    if (Sine < s_Domains.Min[MathsFunc_ArcSin])
    {
        s_Domains.Min[MathsFunc_ArcSin] = Sine;
    }
    if (Sine > s_Domains.Max[MathsFunc_ArcSin])
    {
        s_Domains.Max[MathsFunc_ArcSin] = Sine;
    }

    f64 Result = asin(Sine);
    return Result;
}

static f64 SquareRoot(f64 X)
{
    if (X < s_Domains.Min[MathsFunc_Sqrt])
    {
        s_Domains.Min[MathsFunc_Sqrt] = X;
    }
    if (X > s_Domains.Max[MathsFunc_Sqrt])
    {
        s_Domains.Max[MathsFunc_Sqrt] = X;
    }

    f64 Result = sqrt(X);
    return Result;
}

static void PrintMathsDomains()
{
    printf("Maths function domains:\n");

    for (u32 i = 0; i < MathsFunc_Count; i++)
    {
        maths_func Func = (maths_func)i;
        switch (Func)
        {
            case MathsFunc_Sin:
            {
                printf("Sin: ");
            } break;

            case MathsFunc_Cos:
            {
                printf("Cos: ");
            } break;

            case MathsFunc_ArcSin:
            {
                printf("ArcSin: ");
            } break;


            case MathsFunc_Sqrt:
            {
                printf("SquareRoot: ");
            } break;
        }

        printf("[%.2f, %.2f]\n", s_Domains.Min[i], s_Domains.Max[i]);
    }
}


static f64 ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1)
{ 
    f64 lat1 = Y0;
    f64 lat2 = Y1;
    f64 lon1 = X0;
    f64 lon2 = X1;
    
    f64 dLat = RadiansFromDegrees(lat2 - lat1);
    f64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);
    
    f64 a = Square(Sin(dLat/2.0)) + Cos(lat1)*Cos(lat2)*Square(Sin(dLon/2));
    f64 c = 2.0*ArcSin(SquareRoot(a));
    
    f64 Result = EARTH_RADIUS * c;
    
    return Result;
}

static b32 ApproxEqual(f64 A, f64 B)
{
    constexpr f64 EPSILON = 0.001f;

    f64 Diff = A - B;
    b32 Result = Diff > -EPSILON && Diff < EPSILON;
    return Result;
}


static f64 RefSumHaversine(haver_setup Setup)
{
    f64 Sum = 0.0;
    f64 Coeff = 1.0 / (f64)Setup.NumPairs;
    for (u64 i = 0; i < Setup.NumPairs; i++)
    {
        haver_pair Pair = Setup.Pairs[i];
        f64 Dist = ReferenceHaversine(Pair.X0, Pair.Y0, Pair.X1, Pair.Y1);
        Sum += Coeff * Dist;
    }
    return Sum;
}


static u64 RefVerifyHaversine(haver_setup Setup)
{
    u64 NumErrors = 0;
    for (u64 i = 0; i < Setup.NumPairs; i++)
    {
        haver_pair Pair = Setup.Pairs[i];
        f64 Dist = ReferenceHaversine(Pair.X0, Pair.Y0, Pair.X1, Pair.Y1);
        f64 RefAnswer = Setup.Answers[i];
        if (!ApproxEqual(Dist, RefAnswer))
        {
            NumErrors++;
        }
    }
    return NumErrors;
}


#include <intrin.h>
#include <windows.h>
#include <psapi.h>

#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "bcrypt.lib")

struct windows_metrics
{
	b32 Initialised;
	HANDLE ProcessHandle;
};
static windows_metrics g_WinMetrics;

static void InitOsMetrics()
{
	if (!g_WinMetrics.Initialised)
	{
		g_WinMetrics.Initialised = true;
		g_WinMetrics.ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
	}
}

static u64 ReadNumOsPageFaults()
{
	Assert(g_WinMetrics.Initialised);

	PROCESS_MEMORY_COUNTERS_EX MemoryCounters = {};
	MemoryCounters.cb = sizeof(MemoryCounters);
	GetProcessMemoryInfo(g_WinMetrics.ProcessHandle, (PROCESS_MEMORY_COUNTERS*)&MemoryCounters, sizeof(MemoryCounters));

	return MemoryCounters.PageFaultCount;
}


static u64 GetOSTimerFreq(void)
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

static u64 ReadOSTimer(void)
{
	LARGE_INTEGER Value;
	QueryPerformanceCounter(&Value);
	return Value.QuadPart;
}

static u64 GetMaxOsRandomCount()
{
    return 0xFF'FF'FF'FF;
}

static b32 ReadRandomBytesWinOs(u64 Count, void* Dest)
{
    b32 Result = false;
    if (Count < GetMaxOsRandomCount())
    {
        Result = BCryptGenRandom(0, (BYTE*)Dest, (u32)Count, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0;
    }
    return Result;
}

inline void FillRandom(buffer Dest)
{
    u64 MaxRandom = GetMaxOsRandomCount();
    u64 Offset = 0;
    while (Offset < Dest.Size)
    {
        u64 BytesRead = Dest.Size - Offset;
        if (BytesRead > MaxRandom)
        {
            BytesRead = MaxRandom;
        }

        ReadRandomBytesWinOs(BytesRead, Dest.Memory + Offset);
        Offset += BytesRead;
    }
}

static u64 EstimateCpuFreq(u64 MsToWait)
{
	u64 OsFreq = GetOSTimerFreq();

	u64 CpuStart = __rdtsc();
	u64 OsStart = ReadOSTimer();

	u64 OsEnd = 0;
	u64 OsElapsed = 0;
	u64 OsWaitTime = OsFreq * MsToWait / 1'000;
	while (OsElapsed < OsWaitTime)
	{
		OsEnd = ReadOSTimer();
		OsElapsed = OsEnd - OsStart;
	}

	u64 CpuEnd = __rdtsc();
	u64 CpuElapsed = CpuEnd - CpuStart;
	u64 CpuFreq = 0;
	if (OsElapsed)
	{
		CpuFreq = OsFreq * CpuElapsed / OsElapsed;
	}

	return CpuFreq;
}

static f64 EstimateSecs(u64 CpuElapsed, u64 CpuFreq)
{
	f64 Result = ((f64)CpuElapsed) / CpuFreq;
	return Result;
}

static f64 EstimateSecs(u64 CpuStart, u64 CpuEnd, u64 CpuFreq)
{
	f64 Result = EstimateSecs(CpuEnd - CpuStart, CpuFreq) * 1'000;
	return Result;
}


static f64 EstimateMs(u64 CpuElapsed, u64 CpuFreq)
{
	f64 Secs = ((f64)CpuElapsed) / CpuFreq;
	f64 Result = Secs * 1'000;
	return Result;
}

static f64 EstimateMs(u64 CpuStart, u64 CpuEnd, u64 CpuFreq)
{
	f64 CpuElapsed = CpuEnd - CpuStart;
	return EstimateMs(CpuElapsed, CpuFreq);
}

f64 Percentage(f64 Time, f64 TotalTime)
{
	f64 Result = (Time / TotalTime) * 100.0;
	return Result;
}

f64 Percentage(u64 Amount, u64 Total)
{
	f64 Result = (((f64)Amount) / ((f64)Total)) * 100.0;
	return Result;
}