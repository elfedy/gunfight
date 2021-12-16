typedef unsigned char u8;
typedef unsigned long u32;
typedef float f32;
typedef double f64;

#define arrayLength(A) sizeof(A)/sizeof(A[0])

extern u8 __heap_base;


extern "C" u8 *getHeapBase() {
  return &__heap_base;
}

extern "C" void updateAndRender(f64 timestamp) {
  int triangleBaseOffset;
  f32 *triangleBase = (f32 *) (getHeapBase() + triangleBaseOffset);

  f32 start[6] = {
    0.0f, 0.0f,
    50.0f, 50.0f,
    100.0f, 80.0f,
  };

  for(int i; i < arrayLength(start); i++) {
    *triangleBase++ = start[i];
  }


  //f64 speed = 5.0f / 1000.0f;
  //u32 dx = (u32)(speed * timestamp);
  //triangleX += dx;
}
