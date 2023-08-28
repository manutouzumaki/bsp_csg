#ifndef _DEFINES_H_
#define _DEFINES_H_

typedef int8_t  i8; 
typedef int16_t i16; 
typedef int32_t i32; 
typedef int64_t i64; 

typedef uint8_t  u8; 
typedef uint16_t u16; 
typedef uint32_t u32; 
typedef uint64_t u64; 

typedef float f32;
typedef double f64;

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))
#define ASSERT(condition) if(!(condition)) { *(int *)0 = 0;} 

#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value)*1024LL)
#define GIGABYTES(value) (MEGABYTES(value)*1024LL)
#define TERABYTES(value) (GIGABYTES(value)*1024LL)

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720

#define MAX_DEPTH UINT_MAX
#define MIN_LEAF_SIZE 10

#endif
