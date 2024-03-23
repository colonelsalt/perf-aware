#pragma once



struct profile_anchor
{
	u64 TscElapsed;
	u32 NumHits;
	const char* Label;
};

struct profiler
{
	profile_anchor Anchors[256];

	// Global start and end times
	u64 StartTsc;
	u64 EndTsc;
};
static profiler g_Profiler;


struct profile_block
{
	u64 Start;
	u32 AnchorIndex;
	const char* Label;

	profile_block(const char* BlockLabel, u32 AnchIndex)
	{
		Label = BlockLabel;
		AnchorIndex = AnchIndex;
		Start = __rdtsc();
	}

	~profile_block()
	{
		u64 End = __rdtsc();
		
		profile_anchor* Anchor = g_Profiler.Anchors + AnchorIndex;
		Anchor->TscElapsed += End - Start;
		Anchor->NumHits++;
		Anchor->Label = Label;
	}
};

#define TimeBlock(Name) profile_block __BLOCK(Name, __COUNTER__ + 1)
#define TimeFunction TimeBlock(__func__)

void BeginProfile()
{
	g_Profiler.StartTsc = __rdtsc();
}

void EndAndPrintProfile()
{
	g_Profiler.EndTsc = __rdtsc();
	u64 CpuFreq = EstimateCpuFreq(10);

	f64 TotalTimeMs = EstimateMs(g_Profiler.StartTsc, g_Profiler.EndTsc, CpuFreq);

	printf("\nPerformance metrics:\n");
	printf("Total time: %fms (CPU freq %llu)\n", TotalTimeMs, CpuFreq);

	for (u32 i = 0; i < ArrayCount(g_Profiler.Anchors); i++)
	{
		profile_anchor* Anchor = g_Profiler.Anchors + i;
		if (Anchor->NumHits != 0)
		{
			f64 TimeMs = EstimateMs(Anchor->TscElapsed, CpuFreq);
			f64 Pst = Percentage(TimeMs, TotalTimeMs);

			printf(" %s[%u]: %llu (%.2f%%)\n", Anchor->Label, Anchor->NumHits, Anchor->TscElapsed, Pst);
		}
	}
}