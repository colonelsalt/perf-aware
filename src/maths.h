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

static f64 Abs(f64 X)
{
    f64 Result = X;
    if (X < 0.0)
    {
        Result = -Result;
    }
    return Result;
}

static f64 Square(f64 A)
{
    f64 Result = (A*A);
    return Result;
}

static f64 RadiansFromDegrees(f64 Degrees)
{
    f64 Result = 0.01745329251994329577 * Degrees;
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

typedef double (*maths_func_ptr)(double Input);

static const char* GetFuncName(maths_func Func)
{
    const char* Result = "UNKNOWN";
    switch (Func)
    {
        case MathsFunc_Sin:
        {
            Result = "Sin";
        } break;

        case MathsFunc_Cos:
        {
            Result = "Cos";
        } break;

        case MathsFunc_ArcSin:
        {
            Result = "ArcSin";
        } break;

        case MathsFunc_Sqrt:
        {
            Result = "Sqrt";
        } break;
    }
    return Result;
}

static maths_func_ptr GetReferenceFunction(maths_func Func)
{
    maths_func_ptr Result = nullptr;
    switch (Func)
    {
        case MathsFunc_Sin:
        {
            Result = sin;
        } break;

        case MathsFunc_Cos:
        {
            Result = cos;
        } break;

        case MathsFunc_ArcSin:
        {
            Result = asin;
        } break;

        case MathsFunc_Sqrt:
        {
            Result = sqrt;
        } break;
    }
    return Result;
}

struct maths_func_domain
{
    f64 Min[MathsFunc_Count];
    f64 Max[MathsFunc_Count];
};

static maths_func_domain s_Domains;

#include <cfloat>
static void InitMathsDomains()
{
    for (u32 i = 0; i < MathsFunc_Count; i++)
    {
        s_Domains.Min[i] = DBL_MAX;
        s_Domains.Max[i] = DBL_MIN;
    }
}

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
        const char* Name = GetFuncName(Func);

        printf("%s: [%.2f, %.2f]\n", Name, s_Domains.Min[i], s_Domains.Max[i]);
    }
}