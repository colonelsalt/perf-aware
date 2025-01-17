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
    
    f64 a = Square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*Square(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));
    
    f64 Result = EARTH_RADIUS * c;
    
    return Result;
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