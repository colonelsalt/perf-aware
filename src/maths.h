#include <cstdint>
#include <cmath>
#include <intrin.h>

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

static constexpr f64 PI = 3.14159265358979323846;
static constexpr f64 PI_SQUARED = PI * PI;

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

static f64 Sin_Crt(f64 Angle)
{
    //if (Angle < s_Domains.Min[MathsFunc_Sin])
    //{
    //    s_Domains.Min[MathsFunc_Sin] = Angle;
    //}
    //if (Angle > s_Domains.Max[MathsFunc_Sin])
    //{
    //    s_Domains.Max[MathsFunc_Sin] = Angle;
    //}

    f64 Result = sin(Angle);
    return Result;
}

static f64 Sin_Sondie(f64 Angle)
{
    f64 X = Angle;
    if (Angle < 0)
    {
        X = -Angle;
    }

    f64 A = (-4.0 / PI_SQUARED);
    f64 B = (4.0 / PI);

    f64 Result = A * Square(X) +  B * X;
    if (Angle < 0)
    {
        Result = -Result;
    }

    return Result;
}

static f64 Cos_Crt(f64 Angle)
{
    //if (Angle < s_Domains.Min[MathsFunc_Cos])
    //{
    //    s_Domains.Min[MathsFunc_Cos] = Angle;
    //}
    //if (Angle > s_Domains.Max[MathsFunc_Cos])
    //{
    //    s_Domains.Max[MathsFunc_Cos] = Angle;
    //}

    f64 Result = cos(Angle);
    return Result;
}

static f64 Cos_Sondie(f64 Angle)
{
    f64 X = Angle + PI / 2;
    if (Angle > PI / 2.0)
    {
        X = Angle - PI / 2.0;
    }
    f64 Result = Sin_Sondie(X);
    if (Angle > PI / 2.0)
    {
        Result = -Result;
    }

    return Result;
}

static f64 ArcSin_Crt(f64 Sine)
{
    //if (Sine < s_Domains.Min[MathsFunc_ArcSin])
    //{
    //    s_Domains.Min[MathsFunc_ArcSin] = Sine;
    //}
    //if (Sine > s_Domains.Max[MathsFunc_ArcSin])
    //{
    //    s_Domains.Max[MathsFunc_ArcSin] = Sine;
    //}

    f64 Result = asin(Sine);
    return Result;
}

static f64 ArcSin_Sondie(f64 Sine)
{
    f64 Result = Sine;
    return Result;
}

static f64 SquareRoot_Crt(f64 X)
{
    //if (X < s_Domains.Min[MathsFunc_Sqrt])
    //{
    //    s_Domains.Min[MathsFunc_Sqrt] = X;
    //}
    //if (X > s_Domains.Max[MathsFunc_Sqrt])
    //{
    //    s_Domains.Max[MathsFunc_Sqrt] = X;
    //}

    f64 Result = sqrt(X);
    return Result;
}

static f64 SquareRoot_Sondie(f64 X)
{
    __m128d Xmm = _mm_set_sd(X);
    __m128d Zero = _mm_set_sd(0);
    __m128d Sqrt = _mm_sqrt_sd(Zero, Xmm);

    f64 Result = _mm_cvtsd_f64(Sqrt);
    return Result;
}

struct maths_func_spec
{
    const char* Name;
    maths_func_ptr RefFunc;
    maths_func_ptr CustomFunc;

    f64 MinInput;
    f64 MaxInput;
};

static maths_func_spec GetFuncSpec(maths_func Func)
{
    maths_func_spec Result = {};
    switch (Func)
    {
        case MathsFunc_Sin:
        {
            Result = { "Sin", sin, Sin_Sondie, -3.13, 3.13 };
        } break;

        case MathsFunc_Cos:
        {
            Result = { "Cos", cos, Cos_Sondie, -1.57, 1.57 };
        } break;

        case MathsFunc_ArcSin:
        {
            Result = { "ArcSin", asin, ArcSin_Sondie, 0.00, 1.00 };
        } break;

        case MathsFunc_Sqrt:
        {
            Result = { "Sqrt", sqrt, SquareRoot_Sondie, 0.00, 1.00 };
        } break;
    }

    return Result;
}

static void PrintMathsDomains()
{
    printf("Maths function domains:\n");

    for (u32 i = 0; i < MathsFunc_Count; i++)
    {
        maths_func Func = (maths_func)i;
        const char* Name = GetFuncSpec(Func).Name;

        printf("%s: [%.2f, %.2f]\n", Name, s_Domains.Min[i], s_Domains.Max[i]);
    }
}