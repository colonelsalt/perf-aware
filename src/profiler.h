#pragma once



struct profile_measurement
{
	u64 Elapsed;
	char FunctionName[128];
};
profile_measurement g_Profiles[16];
u32 g_Counter = 0;

struct profile_block
{
	u64 Start;
	const char* FunctionName;

	profile_block(const char* FuncName)
	{
		FunctionName = FuncName;
		Start = __rdtsc();
	}

	~profile_block()
	{
		u64 End = __rdtsc();
		
		profile_measurement* Pm = g_Profiles + g_Counter++;
		Pm->Elapsed = End - Start;
		strcpy(Pm->FunctionName, FunctionName);
	}
};

#define TimeFunction profile_block __BLOCK(__func__)

static u64 s_ProfileStart;

void BeginProfile()
{
	s_ProfileStart = __rdtsc();
}

void EndAndPrintProfile()
{
	u64 ProfileEnd = __rdtsc();
	u64 CpuFreq = EstimateCpuFreq(10);

	f64 TotalTimeMs = EstimateMs(s_ProfileStart, ProfileEnd, CpuFreq);

	printf("\nPerformance metrics:\n");
	printf("Total time: %fms (CPU freq %llu)\n", TotalTimeMs, CpuFreq);

	for (u32 i = 0; i < g_Counter; i++)
	{
		profile_measurement* Profile = g_Profiles + i;

		f64 TimeMs = EstimateMs(Profile->Elapsed, CpuFreq);
		f64 Pst = Percentage(TimeMs, TotalTimeMs);

		printf(" %s: %llu (%.2f%%)\n", Profile->FunctionName, Profile->Elapsed, Pst);
	}
}