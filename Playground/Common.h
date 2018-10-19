#pragma once

#include <cstddef>
#include <stdlib.h>
#include <windows.h>
#include <winnt.h>
#include <atomic>
#include <thread>
#include <mutex>

#ifdef _MSC_VER
#if _MSC_VER >= 1600
#include <cstdint>
#else
typedef __int8              int8_t;
typedef __int16             int16_t;
typedef __int32             int32_t;
typedef __int64             int64_t;
typedef unsigned __int8     uint8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int64    uint64_t;
#endif
#elif __GNUC__ >= 3
#include <cstdint>
#endif

typedef int8_t      s8;
typedef int16_t     s16;
typedef int32_t     s32;
typedef int64_t     s64;
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef std::atomic_int8_t      as8;
typedef std::atomic_int16_t     as16;
typedef std::atomic_int32_t     as32;
typedef std::atomic_int64_t     as64;
typedef std::atomic_uint8_t     au8;
typedef std::atomic_uint16_t    au16;
typedef std::atomic_uint32_t    au32;
typedef std::atomic_uint64_t    au64;

typedef std::thread				worker;

inline float frandom(float min, float max)
{
	return min + ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX))*(max - min));
}

inline u32 urandom(u32 min, u32 max)
{
	return min + rand() % (max - min + 1);
}


#define COMPILER_BARRIER _ReadWriteBarrier()

#define MEMORY_BARRIER std::atomic_thread_fence(std::memory_order_seq_cst);

#define L1_CACHE_LINE_SIZE 64u

#define PROFILE_START auto start = std::chrono::steady_clock::now()

#define PROFILE_END \
auto end = std::chrono::steady_clock::now(); \
auto diff = end - start; \
std::cout << "Time: " << std::chrono::duration<double, std::milli>(diff).count() << std::endl;