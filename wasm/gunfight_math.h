#if !defined(GUNFIGHT_MATH_H)
struct V2 {
  union {
    struct {
      f32 x, y;
    };
    f32 e[2];
  };
};

inline V2 operator*(f32 a, V2 b)
{
  V2 Result;

  Result.x = a*b.x;
  Result.y = a*b.y;

  return Result;
};

inline V2 operator*(V2 b, f32 a)
{
  V2 result = a*b;

  return result;
};

inline V2 & operator*=(V2 &a, f32 b)
{
  a = b * a;

  return a;
}

inline V2 operator-(V2 a)
{
  V2 result;

  result.x = -a.x;
  result.y = -a.y;

  return result;
};

inline V2 operator+(V2 a, V2 b)
{
  V2 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;

  return result;
};

inline V2 & operator+=(V2 &a, V2 b)
{
  a = a + b;
  return a;
};

inline V2 operator-(V2 a, V2 b)
{
  V2 result;

  result.x = a.x - b.x;
  result.y = a.y - b.y;

  return result;
};

inline f32
square(f32 a)
{
  f32 result = a*a;

  return result;
}
#define GUNFIGHT_MATH_H
#endif

