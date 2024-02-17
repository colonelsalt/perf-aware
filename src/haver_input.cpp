#include <stdio.h>
#include <stdlib.h>

#include "haver_util.h"

// Inclusive on both ends
static f64 RandRange(f64 Min, f64 Max)
{
	f64 Scale = rand() / (f64)RAND_MAX;
	f64 Result = Min + Scale * (Max - Min);
	return Result;
}

int main(int ArgC, char** ArgV)
{
	if (ArgC != 3)
	{
		printf("Usage, homie. Expected a random seed (int), and the number of samples (int).\n");
		return 1;
	}
	s32 Seed = atoi(ArgV[1]);
	if (Seed == 0)
	{
		printf("Invalid seed: %s\n", ArgV[1]);
		return 1;
	}
	s32 NumSamples = atoi(ArgV[2]);
	if (NumSamples < 1)
	{
		printf("Can't work with '%s' samples, mate.\n", ArgV[1]);
		return 1;
	}

	FILE* JsonFile = fopen("haver_data.json", "w");
	if (!JsonFile)
	{
		printf("Error opening/creating JSON file\n");
		return 1;
	}
	FILE* AnswerFile = fopen("haver_answer.f64", "wb");
	if (!AnswerFile)
	{
		printf("Error opening/creating the binary answer file.\n");
		return 1;
	}

	srand(Seed);

	fprintf(JsonFile, "{\"pairs\":[\n");

	f64 Sum = 0;
	for (s32 i = 0; i < NumSamples; i++)
	{
		f64 X0 = RandRange(-180, 180);
		f64 X1 = RandRange(-180, 180);
		f64 Y0 = RandRange(-90, 90);
		f64 Y1 = RandRange(-90, 90);

		fprintf(JsonFile, "\t{\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f}", X0, Y0, X1, Y1);
		if (i != NumSamples - 1)
		{
			fprintf(JsonFile, ",\n");
		}

		f64 Haversine = ReferenceHaversine(X0, Y0, X1, Y1);
		fwrite(&Haversine, sizeof(f64), 1, AnswerFile);
		Sum += Haversine;
	}

	// End of file
	fprintf(JsonFile, "\n]}\n");
	f64 Mean = Sum / NumSamples;
	fwrite(&Mean, sizeof(f64), 1, AnswerFile);

	printf("Number of samples: %d\n", NumSamples);
	printf("Expected mean: %f\n", Mean);

	fclose(JsonFile);
	fclose(AnswerFile);
}