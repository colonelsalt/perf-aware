#pragma once



struct profile_anchor
{
	u64 TscElapsedInclusive;
	u64 TscElapsedExclusive;
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
static u32 g_ParentAnchorIndex;


struct profile_block
{
	u64 Start;
	u64 OldTscInclusive;
	u32 AnchorIndex;
	u32 ParentIndex;
	const char* Label;

	profile_block(const char* BlockLabel, u32 AnchIndex)
	{
		Label = BlockLabel;
		AnchorIndex = AnchIndex;

		ParentIndex = g_ParentAnchorIndex;
		g_ParentAnchorIndex = AnchorIndex;

		profile_anchor* Anchor = g_Profiler.Anchors + AnchorIndex;
		OldTscInclusive = Anchor->TscElapsedInclusive;

		Start = __rdtsc();
	}

	~profile_block()
	{
		u64 End = __rdtsc();
		u64 Elapsed = End - Start;

		profile_anchor* Anchor = g_Profiler.Anchors + AnchorIndex;

		Anchor->TscElapsedExclusive += Elapsed;
		Anchor->NumHits++;
		Anchor->Label = Label;

		if (ParentIndex)
		{
			profile_anchor* Parent = g_Profiler.Anchors + ParentIndex;
			Parent->TscElapsedExclusive -= Elapsed;
		}

		Anchor->TscElapsedInclusive = OldTscInclusive + Elapsed;

		g_ParentAnchorIndex = ParentIndex;
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
	u64 TotalTsc = g_Profiler.EndTsc - g_Profiler.StartTsc;
	u64 CpuFreq = EstimateCpuFreq(10);

	f64 TotalTimeMs = EstimateMs(g_Profiler.StartTsc, g_Profiler.EndTsc, CpuFreq);

	printf("\nPerformance metrics:\n");
	printf("Total time: %fms (CPU freq %llu)\n", TotalTimeMs, CpuFreq);

	for (u32 i = 0; i < ArrayCount(g_Profiler.Anchors); i++)
	{
		profile_anchor* Anchor = g_Profiler.Anchors + i;
		if (Anchor->NumHits)
		{
			f64 Pst = Percentage(Anchor->TscElapsedExclusive, TotalTsc);

			printf(" %s[%u]: %llu (%.2f%%", Anchor->Label, Anchor->NumHits, Anchor->TscElapsedExclusive, Pst);
			if (Anchor->TscElapsedInclusive != Anchor->TscElapsedExclusive)
			{
				f64 PstWithChildren = Percentage(Anchor->TscElapsedInclusive, TotalTsc);
				printf(", %.2f%% w/children", PstWithChildren);
			}
			printf(")\n");
		}
	}
}