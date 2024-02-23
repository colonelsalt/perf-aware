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

static constexpr f64 EARTH_RADIUS = 6'372.8;

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