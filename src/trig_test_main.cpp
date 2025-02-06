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
    static constexpr u32 NUM_ITERATIONS = 10'000;

    for (u32 i = 0; i < 1; i++)
    {

        maths_func FuncEnum = (maths_func)i;
        maths_func_spec Spec = GetFuncSpec(FuncEnum);
        
        for (u32 NumTerms = 0; NumTerms < 20; NumTerms++)
        {
            f64 MaxError = 0.0;
            f64 MaxErrorVal = 0.0;
            for (u32 j = 0; j < NUM_ITERATIONS; j++)
            {
                f64 t = (f64)j / (f64)(NUM_ITERATIONS - 1);
                f64 Input = (1 - t) * Spec.MinInput + t * Spec.MaxInput;

                f64 RefOutput = Spec.RefFunc(Input);
                //f64 CustomOutput = Spec.CustomFunc(Input);
                f64 CustomOutput = Sin_Taylor(Input, NumTerms);

                f64 Error = Abs(CustomOutput - RefOutput);
                if (Error > MaxError)
                {
                    MaxError = Error;
                    MaxErrorVal = Input;
                }
            }

            printf("%s (%d Taylor terms) - max error: %f at input %f\n", Spec.Name, NumTerms, MaxError, MaxErrorVal);
        }
    }

    printf("DONE\n");

}