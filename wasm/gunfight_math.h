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

// MOVEMENT
V2 computeNewPosition(V2 initialP, V2 dP, V2 ddP, f32 dt) {
  V2 newP = initialP + 0.5f*ddP*square(dt) + dP * dt;
  return newP;
}

V2 computeNewVelocity(V2 initialDP, V2 ddP, f32 dt) {
  V2 newDP = ddP * dt + initialDP;
  return newDP;
}

// COLLISION
struct CollisionResult {
  bool32 collided;
  V2 newPosition;
};

bool32 rectanglesAreColliding(
    V2 aBottomLeft,
    V2 aTopRight,
    V2 bBottomLeft,
    V2 bTopRight
) {
  bool32 collidingOnXAxis = 
    !(aTopRight.x <= bBottomLeft.x || aBottomLeft.x >= bTopRight.x);
  bool32 collidingOnYAxis = 
    !(aTopRight.y <= bBottomLeft.y || aBottomLeft.y >= bTopRight.y);
  
  bool32 result = collidingOnXAxis && collidingOnYAxis;
  return result;
}

CollisionResult getV2CollisionWithBoundaries(V2 position, V2 min, V2 max) {
  CollisionResult ret = { false, position };
  if(ret.newPosition.x > max.x) {
      ret.newPosition.x = max.x;
      ret.collided = true;
  }
  if(ret.newPosition.x < min.x) {
      ret.newPosition.x = min.x;
      ret.collided = true;
  }
  if(ret.newPosition.y > max.y) {
      ret.newPosition.y = max.y;
      ret.collided = true;
  }
  if(ret.newPosition.y < min.y) {
      ret.newPosition.y = min.y;
      ret.collided = true;
  }
  return ret;
}

#define GUNFIGHT_MATH_H
#endif

