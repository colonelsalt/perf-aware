#include <cstdio>
#include "maths.h"

#include "trig_testcases.cpp"

struct test_cases
{
    const maths_func_test* Tests;
    u32 N;
};

test_cases GetTestCases(maths_func Func)
{
    test_cases Result = {};
    switch (Func)
    {
        case MathsFunc_Sin:
        {
            Result = {SIN_TESTCASES, ArrayCount(SIN_TESTCASES)};
        } break;

        case MathsFunc_Cos:
        {
            Result = {COS_TESTCASES, ArrayCount(COS_TESTCASES)};
        } break;

        case MathsFunc_ArcSin:
        {
            Result = {ARCSIN_TESTCASES, ArrayCount(ARCSIN_TESTCASES)};
        } break;

        case MathsFunc_Sqrt:
        {
            Result = {SQRT_TESTCASES, ArrayCount(SQRT_TESTCASES)};
        } break;
    }
    return Result;
}


int main(int ArgC, char** ArgV)
{
    for (u32 i = 0; i < MathsFunc_Count; i++)
    {
        f64 MaxError = 0.0;

        maths_func Func = (maths_func)i;
        maths_func_ptr FuncPtr = GetReferenceFunction(Func);
        const char* Name = GetFuncName(Func);

        test_cases TestCases = GetTestCases(Func);
        for (u32 j = 0; j < TestCases.N; j++)
        {
            maths_func_test Test = TestCases.Tests[j];

            f64 RefOutput = FuncPtr(Test.Input);
            f64 Error = Abs(Test.Output - RefOutput);
            if (Error > MaxError)
            {
                MaxError = Error;
            }
        }

        printf("%s - max error: %f\n", Name, MaxError);
    }

    printf("DONE\n");

}