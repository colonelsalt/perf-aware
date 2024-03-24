#pragma once

#ifndef PROFILER
#define PROFILER 0
#endif

#if PROFILER

struct profile_anchor
{
	u64 TscElapsedInclusive;
	u64 TscElapsedExclusive;
	u32 NumHits;
	const char* Label;
};

static profile_anchor g_ProfilerAnchors[256];
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

		profile_anchor* Anchor = g_ProfilerAnchors + AnchorIndex;
		OldTscInclusive = Anchor->TscElapsedInclusive;

		Start = __rdtsc();
	}

	~profile_block()
	{
		u64 End = __rdtsc();
		u64 Elapsed = End - Start;

		profile_anchor* Anchor = g_ProfilerAnchors + AnchorIndex;

		Anchor->TscElapsedExclusive += Elapsed;
		Anchor->NumHits++;
		Anchor->Label = Label;

		if (ParentIndex)
		{
			profile_anchor* Parent = g_ProfilerAnchors + ParentIndex;
			Parent->TscElapsedExclusive -= Elapsed;
		}

		Anchor->TscElapsedInclusive = OldTscInclusive + Elapsed;

		g_ParentAnchorIndex = ParentIndex;
	}
};

#define TimeBlock(Name) profile_block __BLOCK(Name, __COUNTER__ + 1)

void PrintAnchorData(u64 TotalElapsedTsc)
{
	for (u32 i = 0; i < ArrayCount(g_ProfilerAnchors); i++)
	{
		profile_anchor* Anchor = g_ProfilerAnchors + i;
		if (Anchor->NumHits)
		{
			f64 Pst = Percentage(Anchor->TscElapsedExclusive, TotalElapsedTsc);

			printf(" %s[%u]: %llu (%.2f%%", Anchor->Label, Anchor->NumHits, Anchor->TscElapsedExclusive, Pst);
			if (Anchor->TscElapsedInclusive != Anchor->TscElapsedExclusive)
			{
				f64 PstWithChildren = Percentage(Anchor->TscElapsedInclusive, TotalElapsedTsc);
				printf(", %.2f%% w/children", PstWithChildren);
			}
			printf(")\n");
		}
	}
}

#else

#define TimeBlock(...)
#define PrintAnchorData(...)

#endif

#define TimeFunction TimeBlock(__func__)

struct profiler
{
	// Global start and end times
	u64 StartTsc;
	u64 EndTsc;
};
static profiler g_Profiler;

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

	PrintAnchorData(TotalTsc);
}