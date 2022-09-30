#if !defined(GUNFIGHT_IMPORTS)

// ENV IMPORTS
extern "C" void envLogF32(f32 number);
extern "C" f32 envRandF32();
extern "C" void envPlayAudioShot();

extern u8 *__heap_base;

#define GUNFIGHT_IMPORTS
#endif
