/************************************************************************************

PublicHeader:   OVR_Kernel.h
Filename    :   OVR_Atomic.h
Content     :   Contains atomic operations and inline fastest locking
                functionality. Will contain #ifdefs for OS efficiency.
                Have non-thread-safe implementaion if not available.
Created     :   September 19, 2012
Notes       : 

Copyright   :   Copyright 2014 Oculus VR, LLC All Rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.2 (the "License"); 
you may not use the Oculus VR Rift SDK except in compliance with the License, 
which is provided at the time of installation or download, or which 
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.2 

Unless required by applicable law or agreed to in writing, the Oculus VR SDK 
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#ifndef OVR_Atomic_h
#define OVR_Atomic_h

#include "OVR_Types.h"

// Include System thread functionality.
#if defined(OVR_OS_MS) && !defined(OVR_OS_MS_MOBILE)
#include "OVR_Win32_IncludeWindows.h"
#else
#include <pthread.h>
#endif

#ifdef OVR_CC_MSVC
#include <intrin.h>
#pragma intrinsic(_ReadBarrier, _WriteBarrier, _ReadWriteBarrier)
#endif

#ifdef OVR_OS_LINUX
#include <atomic>

namespace OVR {
template<class T>
class AtomicTranslator : public std::atomic<T>
{
public:
    AtomicTranslator() :
        std::atomic<T>(T())
    {
    }

    AtomicTranslator(const T &from) :
        std::atomic<T>(from)
    {
    }

    AtomicTranslator(const AtomicTranslator<T> &from) :
        std::atomic<T>(from.Load_Acquire())
    {
    }

    T operator = (const T &from)
    {
        Store_Release(from);
        return from;
    }

    operator T() const
    {
        return Load_Acquire();
    }

    T Exchange_Sync(T& other)
    {
        return std::atomic<T>::exchange(other);
    }

    T Exchange_Sync(const T& other)
    {
        T tmp_val = other;
        return std::atomic<T>::exchange(tmp_val);
    }

    T ExchangeAdd_Sync(const T &val)
    {
        return std::atomic<T>::fetch_add(val);
    }

    T ExchangeAdd_NoSync(const T &val)
    {
        return std::atomic<T>::fetch_add(val, std::memory_order_relaxed);
    }

    bool CompareAndSet_Sync(const T &other, const T &val)
    {
        T tmp_val = other;
        return std::atomic<T>::compare_exchange_strong(tmp_val, val);
    }

    bool CompareAndSet_NoSync(const T &other, const T &val)
    {
        T tmp_val = other;
        return std::atomic<T>::compare_exchange_weak(tmp_val, val);
    }

    T Load_Acquire() const
    {
        return std::atomic<T>::load(std::memory_order_acquire);
    }

    void Store_Release(const T &val)
    {
        std::atomic<T>::store(val, std::memory_order_release);
    }
};
template<class T> using AtomicOps = AtomicTranslator<T>;
template<class T> using AtomicInt = AtomicTranslator<T>;
template<class T> using AtomicPtr = AtomicTranslator<T*>;

#else // OVR_OS_LINUX

namespace OVR {

// ****** Declared classes

// If there is NO thread support we implement AtomicOps and
// Lock objects as no-ops. The other classes are not defined.
template<class C> class AtomicOps;
template<class T> class AtomicInt;
template<class T> class AtomicPtr;

class Lock;


//-----------------------------------------------------------------------------------
// ***** AtomicOps

// Atomic operations are provided by the AtomicOps templates class,
// implemented through system-specific AtomicOpsRaw specializations.
// It provides several fundamental operations such as Exchange, ExchangeAdd
// CompareAndSet, and Store_Release. Each function includes several memory
// synchronization versions, important for multiprocessing CPUs with weak
// memory consistency. The following memory fencing strategies are supported:
//
//  - NoSync.  No memory synchronization is done for atomic op.
//  - Release. All other memory writes are completed before atomic op
//             writes its results.
//  - Acquire. Further memory reads are forced to wait until atomic op
//             executes, guaranteeing that the right values will be seen.
//  - Sync.    A combination of Release and Acquire.


// *** AtomicOpsRaw

// AtomicOpsRaw is a specialized template that provides atomic operations 
// used by AtomicOps. This class has two fundamental qualities: (1) it
// defines a type T of correct size, and (2) provides operations that work
// atomically, such as Exchange_Sync and CompareAndSet_Release.

// AtomicOpsRawBase class contains shared constants/classes for AtomicOpsRaw.
// The primary thing is does is define sync class objects, whose destructor and
// constructor provide places to insert appropriate synchronization calls, on 
// systems where such calls are necessary. So far, the breakdown is as follows:
// 
//  - X86 systems don't need custom syncs, since their exchange/atomic
//    instructions are implicitly synchronized.
//  - PowerPC requires lwsync/isync instructions that can use this mechanism.
//  - If some other systems require a mechanism where syncing type is associated
//    with a particular instruction, the default implementation (which implements
//    all Sync, Acquire, and Release modes in terms of NoSync and fence) may not
//    work. Ii that case it will need to be #ifdef-ed conditionally.

struct AtomicOpsRawBase
{
#if !defined(OVR_ENABLE_THREADS) || defined(OVR_CPU_X86) || defined(OVR_CPU_X86_64)
    // Need to have empty constructor to avoid class 'unused' variable warning.
    struct FullSync { inline FullSync() { } };
    struct AcquireSync { inline AcquireSync() { } };
    struct ReleaseSync { inline ReleaseSync() { } };

#elif defined(OVR_CPU_PPC64) || defined(OVR_CPU_PPC)
    struct FullSync { inline FullSync() { asm volatile("sync\n"); } ~FullSync() { asm volatile("isync\n"); } };
    struct AcquireSync { inline AcquireSync() { } ~AcquireSync() { asm volatile("isync\n"); } };
    struct ReleaseSync { inline ReleaseSync() { asm volatile("sync\n"); } };

#elif defined(OVR_CPU_MIPS)
    struct FullSync { inline FullSync() { asm volatile("sync\n"); } ~FullSync() { asm volatile("sync\n"); } };
    struct AcquireSync { inline AcquireSync() { } ~AcquireSync() { asm volatile("sync\n"); } };
    struct ReleaseSync { inline ReleaseSync() { asm volatile("sync\n"); } };

#elif defined(OVR_CPU_ARM) // Includes Android and iOS.
    struct FullSync { inline FullSync() { asm volatile("dmb\n"); } ~FullSync() { asm volatile("dmb\n"); } };
    struct AcquireSync { inline AcquireSync() { } ~AcquireSync() { asm volatile("dmb\n"); } };
    struct ReleaseSync { inline ReleaseSync() { asm volatile("dmb\n"); } };

#elif defined(OVR_CC_GNU) && (__GNUC__ >= 4)
    // __sync functions are already full sync
    struct FullSync { inline FullSync() { } };
    struct AcquireSync { inline AcquireSync() { } };
    struct ReleaseSync { inline ReleaseSync() { } };
#endif
};


// 4-Byte raw data atomic op implementation class.
struct AtomicOpsRaw_4ByteImpl : public AtomicOpsRawBase
{
#if !defined(OVR_ENABLE_THREADS)

    // Provide a type for no-thread-support cases. Used by AtomicOpsRaw_DefImpl.
    typedef uint32_t T;   

    // *** Thread - Safe Atomic Versions.

#elif defined(OVR_OS_MS) 

    // Use special defined for VC6, where volatile is not used and
    // InterlockedCompareExchange is declared incorrectly.
    typedef LONG T;      
#if defined(OVR_CC_MSVC) && (OVR_CC_MSVC < 1300)
    typedef T* InterlockTPtr;
    typedef LPVOID ET;
    typedef ET* InterlockETPtr;
#else
    typedef volatile T* InterlockTPtr;
    typedef T ET;
    typedef InterlockTPtr InterlockETPtr;
#endif
    inline static T     Exchange_NoSync(volatile T* p, T val)            { return InterlockedExchange((InterlockTPtr)p, val); }
    inline static T     ExchangeAdd_NoSync(volatile T* p, T val)         { return InterlockedExchangeAdd((InterlockTPtr)p, val); }
    inline static bool  CompareAndSet_NoSync(volatile T* p, T c, T val)  { return InterlockedCompareExchange((InterlockETPtr)p, (ET)val, (ET)c) == (ET)c; }

#elif defined(OVR_CPU_PPC64) || defined(OVR_CPU_PPC)
    typedef uint32_t T;
    static inline uint32_t Exchange_NoSync(volatile uint32_t *i, uint32_t j)
    {
        uint32_t ret;

        asm volatile("1:\n\t"
                     "lwarx  %[r],0,%[i]\n\t"
                     "stwcx. %[j],0,%[i]\n\t"
                     "bne-   1b\n"
                     : "+m" (*i), [r] "=&b" (ret) : [i] "b" (i), [j] "b" (j) : "cc", "memory");

        return ret;
    }

    static inline uint32_t ExchangeAdd_NoSync(volatile uint32_t *i, uint32_t j)
    {
        uint32_t dummy, ret;

        asm volatile("1:\n\t"
                     "lwarx  %[r],0,%[i]\n\t"
                     "add    %[o],%[r],%[j]\n\t"
                     "stwcx. %[o],0,%[i]\n\t"
                     "bne-   1b\n"
                     : "+m" (*i), [r] "=&b" (ret), [o] "=&r" (dummy) : [i] "b" (i), [j] "b" (j) : "cc", "memory");

        return ret;
    }

    static inline bool     CompareAndSet_NoSync(volatile uint32_t *i, uint32_t c, uint32_t value)
    {
        uint32_t ret;

        asm volatile("1:\n\t"
                     "lwarx  %[r],0,%[i]\n\t"
                     "cmpw   0,%[r],%[cmp]\n\t"
                     "mfcr   %[r]\n\t"
                     "bne-   2f\n\t"
                     "stwcx. %[val],0,%[i]\n\t"
                     "bne-   1b\n\t"
                     "2:\n"
                     : "+m" (*i), [r] "=&b" (ret) : [i] "b" (i), [cmp] "b" (c), [val] "b" (value) : "cc", "memory");

        return (ret & 0x20000000) ? 1 : 0;
    }

#elif defined(OVR_CPU_MIPS)
    typedef uint32_t T;

    static inline uint32_t Exchange_NoSync(volatile uint32_t *i, uint32_t j)
    {
        uint32_t ret;

        asm volatile("1:\n\t"
                     "ll     %[r],0(%[i])\n\t"
                     "sc     %[j],0(%[i])\n\t"
                     "beq    %[j],$0,1b\n\t"
                     "nop    \n"
                     : "+m" (*i), [r] "=&d" (ret) : [i] "d" (i), [j] "d" (j) : "cc", "memory");

        return ret;
    }

    static inline uint32_t ExchangeAdd_NoSync(volatile uint32_t *i, uint32_t j)
    {
        uint32_t ret;

        asm volatile("1:\n\t"
                     "ll     %[r],0(%[i])\n\t"
                     "addu   %[j],%[r],%[j]\n\t"
                     "sc     %[j],0(%[i])\n\t"
                     "beq    %[j],$0,1b\n\t"
                     "nop    \n"
                     : "+m" (*i), [r] "=&d" (ret) : [i] "d" (i), [j] "d" (j) : "cc", "memory");

        return ret;
    }

    static inline bool     CompareAndSet_NoSync(volatile uint32_t *i, uint32_t c, uint32_t value)
    {
        uint32_t ret, dummy;

        asm volatile("1:\n\t"
                     "move   %[r],$0\n\t"
                     "ll     %[o],0(%[i])\n\t"
                     "bne    %[o],%[c],2f\n\t"
                     "move   %[r],%[v]\n\t"
                     "sc     %[r],0(%[i])\n\t"
                     "beq    %[r],$0,1b\n\t"
                     "nop    \n\t"
                     "2:\n"
                     : "+m" (*i),[r] "=&d" (ret), [o] "=&d" (dummy) : [i] "d" (i), [c] "d" (c), [v] "d" (value)
                     : "cc", "memory");

        return ret;
    }

#elif defined(OVR_CPU_ARM) && defined(OVR_CC_ARM)
    typedef uint32_t T;

    static inline uint32_t Exchange_NoSync(volatile uint32_t *i, uint32_t j)
    {
        for(;;)
        {
            T r = __ldrex(i);
            if (__strex(j, i) == 0)
                return r;
        }
    }
    static inline uint32_t ExchangeAdd_NoSync(volatile uint32_t *i, uint32_t j)
    {
        for(;;)
        {
            T r = __ldrex(i);
            if (__strex(r + j, i) == 0)
                return r;
        }
    }

    static inline bool     CompareAndSet_NoSync(volatile uint32_t *i, uint32_t c, uint32_t value)
    {
        for(;;)
        {
            T r = __ldrex(i);
            if (r != c)
                return 0;
            if (__strex(value, i) == 0)
                return 1;
        }
    }

#elif defined(OVR_CPU_ARM)
    typedef uint32_t T;

    static inline uint32_t Exchange_NoSync(volatile uint32_t *i, uint32_t j)
    {
        uint32_t ret, dummy;

        asm volatile("1:\n\t"
            "ldrex  %[r],[%[i]]\n\t"
            "strex  %[t],%[j],[%[i]]\n\t"
            "cmp    %[t],#0\n\t"
            "bne    1b\n\t"
            : "+m" (*i), [r] "=&r" (ret), [t] "=&r" (dummy) : [i] "r" (i), [j] "r" (j) : "cc", "memory");

        return ret;
    }

    static inline uint32_t ExchangeAdd_NoSync(volatile uint32_t *i, uint32_t j)
    {
        uint32_t ret, dummy, test;

        asm volatile("1:\n\t"
            "ldrex  %[r],[%[i]]\n\t"
            "add    %[o],%[r],%[j]\n\t"
            "strex  %[t],%[o],[%[i]]\n\t"
            "cmp    %[t],#0\n\t"
            "bne    1b\n\t"
            : "+m" (*i), [r] "=&r" (ret), [o] "=&r" (dummy), [t] "=&r" (test)  : [i] "r" (i), [j] "r" (j) : "cc", "memory");

        return ret;
    }

    static inline bool     CompareAndSet_NoSync(volatile uint32_t *i, uint32_t c, uint32_t value)
    {
        uint32_t ret = 1, dummy, test;

        asm volatile("1:\n\t"
            "ldrex  %[o],[%[i]]\n\t"
            "cmp    %[o],%[c]\n\t"
            "bne    2f\n\t"
            "strex  %[r],%[v],[%[i]]\n\t"
            "cmp    %[r],#0\n\t"
            "bne    1b\n\t"
            "2:\n"
            : "+m" (*i),[r] "=&r" (ret), [o] "=&r" (dummy), [t] "=&r" (test) : [i] "r" (i), [c] "r" (c), [v] "r" (value)
            : "cc", "memory");

        return !ret;
    }

#elif defined(OVR_CPU_X86)
    typedef uint32_t T;

    static inline uint32_t Exchange_NoSync(volatile uint32_t *i, uint32_t j)
    {
        asm volatile("xchgl %1,%[i]\n"
                     : "+m" (*i), "=q" (j) : [i] "m" (*i), "1" (j) : "cc", "memory");

        return j;
    }

    static inline uint32_t ExchangeAdd_NoSync(volatile uint32_t *i, uint32_t j)
    {
        asm volatile("lock; xaddl %1,%[i]\n"
                     : "+m" (*i), "+q" (j) : [i] "m" (*i) : "cc", "memory");

        return j;
    }

    static inline bool     CompareAndSet_NoSync(volatile uint32_t *i, uint32_t c, uint32_t value)
    {
        uint32_t ret;

        asm volatile("lock; cmpxchgl %[v],%[i]\n"
                     : "+m" (*i), "=a" (ret) : [i] "m" (*i), "1" (c), [v] "q" (value) : "cc", "memory");

        return (ret == c);
    }

#elif defined(OVR_CC_GNU) && (__GNUC__ >= 4 && __GNUC_MINOR__ >= 1)

    typedef uint32_t T;

    static inline T   Exchange_NoSync(volatile T *i, T j)
    {
        T v;
        do {
            v = *i;
        } while (!__sync_bool_compare_and_swap(i, v, j));
        return v;
    }

    static inline T   ExchangeAdd_NoSync(volatile T *i, T j)
    {
        return __sync_fetch_and_add(i, j);
    }

    static inline bool     CompareAndSet_NoSync(volatile T *i, T c, T value)
    {
        return __sync_bool_compare_and_swap(i, c, value);
    }

#endif // OS
};


// 8-Byte raw data data atomic op implementation class.
// Currently implementation is provided only on systems with 64-bit pointers.
struct AtomicOpsRaw_8ByteImpl : public AtomicOpsRawBase
{    
#if !defined(OVR_64BIT_POINTERS) || !defined(OVR_ENABLE_THREADS)

    // Provide a type for no-thread-support cases. Used by AtomicOpsRaw_DefImpl.
    typedef uint64_t T;

    // *** Thread - Safe OS specific versions.
#elif defined(OVR_OS_MS)

    // This is only for 64-bit systems.
    typedef LONG64      T;
    typedef volatile T* InterlockTPtr;    
    inline static T     Exchange_NoSync(volatile T* p, T val)            { return InterlockedExchange64((InterlockTPtr)p, val); }
    inline static T     ExchangeAdd_NoSync(volatile T* p, T val)         { return InterlockedExchangeAdd64((InterlockTPtr)p, val); }
    inline static bool  CompareAndSet_NoSync(volatile T* p, T c, T val)  { return InterlockedCompareExchange64((InterlockTPtr)p, val, c) == c; }

#elif defined(OVR_CPU_PPC64)
 
    typedef uint64_t T;

    static inline uint64_t Exchange_NoSync(volatile uint64_t *i, uint64_t j)
    {
        uint64_t dummy, ret;

        asm volatile("1:\n\t"
                     "ldarx  %[r],0,%[i]\n\t"
                     "mr     %[o],%[j]\n\t"
                     "stdcx. %[o],0,%[i]\n\t"
                     "bne-   1b\n"
                     : "+m" (*i), [r] "=&b" (ret), [o] "=&r" (dummy) : [i] "b" (i), [j] "b" (j) : "cc");

        return ret;
    }

    static inline uint64_t ExchangeAdd_NoSync(volatile uint64_t *i, uint64_t j)
    {
        uint64_t dummy, ret;

        asm volatile("1:\n\t"
                     "ldarx  %[r],0,%[i]\n\t"
                     "add    %[o],%[r],%[j]\n\t"
                     "stdcx. %[o],0,%[i]\n\t"
                     "bne-   1b\n"
                     : "+m" (*i), [r] "=&b" (ret), [o] "=&r" (dummy) : [i] "b" (i), [j] "b" (j) : "cc");

        return ret;
    }

    static inline bool     CompareAndSet_NoSync(volatile uint64_t *i, uint64_t c, uint64_t value)
    {
        uint64_t ret, dummy;

        asm volatile("1:\n\t"
                     "ldarx  %[r],0,%[i]\n\t"
                     "cmpw   0,%[r],%[cmp]\n\t"
                     "mfcr   %[r]\n\t"
                     "bne-   2f\n\t"
                     "stdcx. %[val],0,%[i]\n\t"
                     "bne-   1b\n\t"
                     "2:\n"
                     : "+m" (*i), [r] "=&b" (ret), [o] "=&r" (dummy) : [i] "b" (i), [cmp] "b" (c), [val] "b" (value) : "cc");

        return (ret & 0x20000000) ? 1 : 0;
    }

#elif defined(OVR_CC_GNU) && (__GNUC__ >= 4 && __GNUC_MINOR__ >= 1)

    typedef uint64_t T;

    static inline T   Exchange_NoSync(volatile T *i, T j)
    {
        T v;
        do {
            v = *i;
        } while (!__sync_bool_compare_and_swap(i, v, j));
        return v;
    }

    static inline T   ExchangeAdd_NoSync(volatile T *i, T j)
    {
        return __sync_fetch_and_add(i, j);
    }

    static inline bool     CompareAndSet_NoSync(volatile T *i, T c, T value)
    {
        return __sync_bool_compare_and_swap(i, c, value);
    }

#endif // OS
};


// Default implementation for AtomicOpsRaw; provides implementation of mem-fenced
// atomic operations where fencing is done with a sync object wrapped around a NoSync
// operation implemented in the base class. If such implementation is not possible
// on a given platform, #ifdefs can be used to disable it and then op functions can be
// implemented individually in the appropriate AtomicOpsRaw<size> class.

template<class O>
struct AtomicOpsRaw_DefImpl : public O
{
    typedef typename O::T O_T;
    typedef typename O::FullSync    O_FullSync;
    typedef typename O::AcquireSync O_AcquireSync;
    typedef typename O::ReleaseSync O_ReleaseSync;

    // If there is no thread support, provide the default implementation. In this case,
    // the base class (0) must still provide the T declaration.
#ifndef OVR_ENABLE_THREADS

    // Atomic exchange of val with argument. Returns old val.
    inline static O_T   Exchange_NoSync(volatile O_T* p, O_T val)           { O_T old = *p; *p = val; return old; }
    // Adds a new val to argument; returns its old val.
    inline static O_T   ExchangeAdd_NoSync(volatile O_T* p, O_T val)        { O_T old = *p; *p += val; return old; }
    // Compares the argument data with 'c' val.
    // If succeeded, stores val int '*p' and returns true; otherwise returns false.
    inline static bool  CompareAndSet_NoSync(volatile O_T* p, O_T c, O_T val) { if (*p==c) { *p = val; return 1; } return 0; }

#endif

    // If NoSync wrapped implementation may not be possible, it this block should be
    //  replaced with per-function implementation in O.
    // "AtomicOpsRaw_DefImpl<O>::" prefix in calls below.
    inline static O_T   Exchange_Sync(volatile O_T* p, O_T val)                { O_FullSync    sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::Exchange_NoSync(p, val); }
    inline static O_T   Exchange_Release(volatile O_T* p, O_T val)             { O_ReleaseSync sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::Exchange_NoSync(p, val); }
    inline static O_T   Exchange_Acquire(volatile O_T* p, O_T val)             { O_AcquireSync sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::Exchange_NoSync(p, val); }  
    inline static O_T   ExchangeAdd_Sync(volatile O_T* p, O_T val)             { O_FullSync    sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::ExchangeAdd_NoSync(p, val); }
    inline static O_T   ExchangeAdd_Release(volatile O_T* p, O_T val)          { O_ReleaseSync sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::ExchangeAdd_NoSync(p, val); }
    inline static O_T   ExchangeAdd_Acquire(volatile O_T* p, O_T val)          { O_AcquireSync sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::ExchangeAdd_NoSync(p, val); }
    inline static bool  CompareAndSet_Sync(volatile O_T* p, O_T c, O_T val)    { O_FullSync    sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::CompareAndSet_NoSync(p,c,val); }
    inline static bool  CompareAndSet_Release(volatile O_T* p, O_T c, O_T val) { O_ReleaseSync sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::CompareAndSet_NoSync(p,c,val); }
    inline static bool  CompareAndSet_Acquire(volatile O_T* p, O_T c, O_T val) { O_AcquireSync sync; OVR_UNUSED(sync); return AtomicOpsRaw_DefImpl<O>::CompareAndSet_NoSync(p,c,val); }

    // Loads and stores with memory fence. These have only the relevant versions.
#ifdef OVR_CPU_X86
    // On X86, Store_Release is implemented as exchange. Note that we can also
    // consider 'sfence' in the future, although it is not as compatible with older CPUs.
    inline static void  Store_Release(volatile O_T* p, O_T val)  { Exchange_Release(p, val); }
#else
    inline static void  Store_Release(volatile O_T* p, O_T val)  { O_ReleaseSync sync; OVR_UNUSED(sync); *p = val; }
#endif
    inline static O_T   Load_Acquire(const volatile O_T* p)
    {
        O_AcquireSync sync;
        OVR_UNUSED(sync);

#if defined(OVR_CC_MSVC)
        _ReadBarrier(); // Compiler fence and load barrier
#elif defined(OVR_CC_INTEL)
        __memory_barrier(); // Compiler fence
#else
        // GCC-compatible:
        asm volatile ("" : : : "memory"); // Compiler fence
#endif

        return *p;
    }
};


template<int size>
struct AtomicOpsRaw : public AtomicOpsRawBase { };

template<>
struct AtomicOpsRaw<4> : public AtomicOpsRaw_DefImpl<AtomicOpsRaw_4ByteImpl>
{   
    // Ensure that assigned type size is correct.
    AtomicOpsRaw()
    { OVR_COMPILER_ASSERT(sizeof(AtomicOpsRaw_DefImpl<AtomicOpsRaw_4ByteImpl>::T) == 4); }
};
template<>
struct AtomicOpsRaw<8> : public AtomicOpsRaw_DefImpl<AtomicOpsRaw_8ByteImpl>
{
    AtomicOpsRaw()
    { OVR_COMPILER_ASSERT(sizeof(AtomicOpsRaw_DefImpl<AtomicOpsRaw_8ByteImpl>::T) == 8); }
};


// *** AtomicOps - implementation of atomic Ops for specified class

// Implements atomic ops on a class, provided that the object is either
// 4 or 8 bytes in size (depending on the AtomicOpsRaw specializations
// available). Relies on AtomicOpsRaw for much of implementation.

template<class C>
class AtomicOps
{
    typedef AtomicOpsRaw<sizeof(C)>       Ops;
    typedef typename Ops::T               T;
    typedef volatile typename Ops::T*     PT;
    // We cast through unions to (1) avoid pointer size compiler warnings
    // and (2) ensure that there are no problems with strict pointer aliasing.
    union C2T_union { C c; T t; };

public:
    // General purpose implementation for standard syncs.    
    inline static C     Exchange_Sync(volatile C* p, C val)             { C2T_union u; u.c = val; u.t = Ops::Exchange_Sync((PT)p, u.t); return u.c; }
    inline static C     Exchange_Release(volatile C* p, C val)          { C2T_union u; u.c = val; u.t = Ops::Exchange_Release((PT)p, u.t); return u.c; }
    inline static C     Exchange_Acquire(volatile C* p, C val)          { C2T_union u; u.c = val; u.t = Ops::Exchange_Acquire((PT)p, u.t); return u.c; }
    inline static C     Exchange_NoSync(volatile C* p, C val)           { C2T_union u; u.c = val; u.t = Ops::Exchange_NoSync((PT)p, u.t); return u.c; }
    inline static C     ExchangeAdd_Sync(volatile C* p, C val)          { C2T_union u; u.c = val; u.t = Ops::ExchangeAdd_Sync((PT)p, u.t); return u.c; }
    inline static C     ExchangeAdd_Release(volatile C* p, C val)       { C2T_union u; u.c = val; u.t = Ops::ExchangeAdd_Release((PT)p, u.t); return u.c; }
    inline static C     ExchangeAdd_Acquire(volatile C* p, C val)       { C2T_union u; u.c = val; u.t = Ops::ExchangeAdd_Acquire((PT)p, u.t); return u.c; }
    inline static C     ExchangeAdd_NoSync(volatile C* p, C val)        { C2T_union u; u.c = val; u.t = Ops::ExchangeAdd_NoSync((PT)p, u.t); return u.c; }
    inline static bool  CompareAndSet_Sync(volatile C* p, C c, C val)   { C2T_union u,cu; u.c = val; cu.c = c; return Ops::CompareAndSet_Sync((PT)p, cu.t, u.t); }
    inline static bool  CompareAndSet_Release(volatile C* p, C c, C val){ C2T_union u,cu; u.c = val; cu.c = c; return Ops::CompareAndSet_Release((PT)p, cu.t, u.t); }
    inline static bool  CompareAndSet_Acquire(volatile C* p, C c, C val){ C2T_union u,cu; u.c = val; cu.c = c; return Ops::CompareAndSet_Acquire((PT)p, cu.t, u.t); }
    inline static bool  CompareAndSet_NoSync(volatile C* p, C c, C val) { C2T_union u,cu; u.c = val; cu.c = c; return Ops::CompareAndSet_NoSync((PT)p, cu.t, u.t); }

    // Loads and stores with memory fence. These have only the relevant versions.    
    inline static void  Store_Release(volatile C* p, C val)             { C2T_union u; u.c = val; Ops::Store_Release((PT)p, u.t); }    
    inline static C     Load_Acquire(const volatile C* p)               { C2T_union u; u.t = Ops::Load_Acquire((PT)p); return u.c; }

    // Deprecated typo error:
    inline static bool  CompareAndSet_Relse(volatile C* p, C c, C val){ C2T_union u,cu; u.c = val; cu.c = c; return Ops::CompareAndSet_Acquire((PT)p, cu.t, u.t); }
};



// Atomic value base class - implements operations shared for integers and pointers.
template<class T>
class AtomicValueBase
{
protected:
    typedef AtomicOps<T> Ops;
public:

    volatile T  Value;

    inline AtomicValueBase()                  { }
    explicit inline AtomicValueBase(T val)    { Ops::Store_Release(&Value, val); }

    // Most libraries (TBB and Joshua Scholar's) library do not do Load_Acquire
    // here, since most algorithms do not require atomic loads. Needs some research.    
    inline operator T() const { return Value; }

    // *** Standard Atomic inlines
    inline T     Exchange_Sync(T val)               { return Ops::Exchange_Sync(&Value,  val); }
    inline T     Exchange_Release(T val)            { return Ops::Exchange_Release(&Value, val); }
    inline T     Exchange_Acquire(T val)            { return Ops::Exchange_Acquire(&Value, val); }
    inline T     Exchange_NoSync(T val)             { return Ops::Exchange_NoSync(&Value, val); }
    inline bool  CompareAndSet_Sync(T c, T val)     { return Ops::CompareAndSet_Sync(&Value, c, val); }
    inline bool  CompareAndSet_Release(T c, T val)  { return Ops::CompareAndSet_Release(&Value, c, val); }
    inline bool  CompareAndSet_Acquire(T c, T val)  { return Ops::CompareAndSet_Acquire(&Value, c, val); }
    inline bool  CompareAndSet_NoSync(T c, T val)   { return Ops::CompareAndSet_NoSync(&Value, c, val); }
    // Load & Store.
    inline void  Store_Release(T val)               { Ops::Store_Release(&Value, val); }
    inline T     Load_Acquire() const               { return Ops::Load_Acquire(&Value);  }
};


// ***** AtomicPtr - Atomic pointer template

// This pointer class supports atomic assignments with release,
// increment / decrement operations, and conditional compare + set.

template<class T>
class AtomicPtr : public AtomicValueBase<T*>
{
    typedef typename AtomicValueBase<T*>::Ops Ops;

public:
    // Initialize pointer value to 0 by default; use Store_Release only with explicit constructor.
    inline AtomicPtr() : AtomicValueBase<T*>()                     { this->Value = 0; }
    explicit inline AtomicPtr(T* val) : AtomicValueBase<T*>(val)   { }
        
    // Pointer access.
    inline T* operator -> () const     { return this->Load_Acquire(); }

    // It looks like it is convenient to have Load_Acquire characteristics
    // for this, since that is convenient for algorithms such as linked
    // list traversals that can be added to bu another thread.
    inline operator T* () const        { return this->Load_Acquire(); }


    // *** Standard Atomic inlines (applicable to pointers)

    // ExhangeAdd considers pointer size for pointers.
    template<class I>
    inline T*     ExchangeAdd_Sync(I incr)      { return Ops::ExchangeAdd_Sync(&this->Value, ((T*)0) + incr); }
    template<class I>
    inline T*     ExchangeAdd_Release(I incr)   { return Ops::ExchangeAdd_Release(&this->Value, ((T*)0) + incr); }
    template<class I>
    inline T*     ExchangeAdd_Acquire(I incr)   { return Ops::ExchangeAdd_Acquire(&this->Value, ((T*)0) + incr); }
    template<class I>
    inline T*     ExchangeAdd_NoSync(I incr)    { return Ops::ExchangeAdd_NoSync(&this->Value, ((T*)0) + incr); }

    // *** Atomic Operators

    inline T* operator = (T* val)  { this->Store_Release(val); return val; }

    template<class I>
    inline T* operator += (I val) { return ExchangeAdd_Sync(val) + val; }
    template<class I>
    inline T* operator -= (I val) { return operator += (-val); }

    inline T* operator ++ ()      { return ExchangeAdd_Sync(1) + 1; }
    inline T* operator -- ()      { return ExchangeAdd_Sync(-1) - 1; }
    inline T* operator ++ (int)   { return ExchangeAdd_Sync(1); }
    inline T* operator -- (int)   { return ExchangeAdd_Sync(-1); }
};


// ***** AtomicInt - Atomic integer template

// Implements an atomic integer type; the exact type to use is provided 
// as an argument. Supports atomic Acquire / Release semantics, atomic
// arithmetic operations, and atomic conditional compare + set.

template<class T>
class AtomicInt : public AtomicValueBase<T>
{
    typedef typename AtomicValueBase<T>::Ops Ops;

public:
    inline AtomicInt() : AtomicValueBase<T>()                     { }
    explicit inline AtomicInt(T val) : AtomicValueBase<T>(val)    { }


    // *** Standard Atomic inlines (applicable to int)   
    inline T     ExchangeAdd_Sync(T val)            { return Ops::ExchangeAdd_Sync(&this->Value, val); }
    inline T     ExchangeAdd_Release(T val)         { return Ops::ExchangeAdd_Release(&this->Value, val); }
    inline T     ExchangeAdd_Acquire(T val)         { return Ops::ExchangeAdd_Acquire(&this->Value, val); }
    inline T     ExchangeAdd_NoSync(T val)          { return Ops::ExchangeAdd_NoSync(&this->Value, val); }
    // These increments could be more efficient because they don't return a value.
    inline void  Increment_Sync()                   { ExchangeAdd_Sync((T)1); }
    inline void  Increment_Release()                { ExchangeAdd_Release((T)1); }
    inline void  Increment_Acquire()                { ExchangeAdd_Acquire((T)1); }    
    inline void  Increment_NoSync()                 { ExchangeAdd_NoSync((T)1); }

    // *** Atomic Operators

    inline T operator = (T val)  { this->Store_Release(val); return val; }
    inline T operator += (T val) { return ExchangeAdd_Sync(val) + val; }
    inline T operator -= (T val) { return ExchangeAdd_Sync(0 - val) - val; }

    inline T operator ++ ()      { return ExchangeAdd_Sync((T)1) + 1; }
    inline T operator -- ()      { return ExchangeAdd_Sync(((T)0)-1) - 1; }
    inline T operator ++ (int)   { return ExchangeAdd_Sync((T)1); }
    inline T operator -- (int)   { return ExchangeAdd_Sync(((T)0)-1); }

    // More complex atomic operations. Leave it to compiler whether to optimize them or not.
    T operator &= (T arg)
    {
        T comp, newVal;
        do {
            comp   = this->Value;
            newVal = comp & arg;
        } while(!this->CompareAndSet_Sync(comp, newVal));
        return newVal;
    }

    T operator |= (T arg)
    {
        T comp, newVal;
        do {
            comp   = this->Value;
            newVal = comp | arg;
        } while(!this->CompareAndSet_Sync(comp, newVal));
        return newVal;
    }

    T operator ^= (T arg)
    {
        T comp, newVal;
        do {
            comp   = this->Value;
            newVal = comp ^ arg;
        } while(!this->CompareAndSet_Sync(comp, newVal));
        return newVal;
    }

    T operator *= (T arg)
    {
        T comp, newVal;
        do {
            comp   = this->Value;
            newVal = comp * arg;
        } while(!this->CompareAndSet_Sync(comp, newVal));
        return newVal;
    }

    T operator /= (T arg)
    {
        T comp, newVal;
        do {
            comp   = this->Value;
            newVal = comp / arg;
        } while(!CompareAndSet_Sync(comp, newVal));
        return newVal;
    }

    T operator >>= (unsigned bits)
    {
        T comp, newVal;
        do {
            comp   = this->Value;
            newVal = comp >> bits;
        } while(!CompareAndSet_Sync(comp, newVal));
        return newVal;
    }

    T operator <<= (unsigned bits)
    {
        T comp, newVal;
        do {
            comp   = this->Value;
            newVal = comp << bits;
        } while(!this->CompareAndSet_Sync(comp, newVal));
        return newVal;
    }
};

#endif // OVR_OS_LINUX

//-----------------------------------------------------------------------------------
// ***** Lock

// Lock is a simplest and most efficient mutual-exclusion lock class.
// Unlike Mutex, it cannot be waited on.

class Lock
{    
#if !defined(OVR_ENABLE_THREADS)

public:
    // With no thread support, lock does nothing.
    inline Lock() { }
    inline Lock(unsigned) { }
    inline ~Lock() { }    
    inline void DoLock() { }
    inline void Unlock() { }

   // Windows.   
#elif defined(OVR_OS_MS)

    CRITICAL_SECTION cs;
public:   
    Lock(unsigned spinCount = 10000);   // Mutexes with non-zero spin counts usually result in better performance.
    ~Lock();
    // Locking functions.
    inline void DoLock()    { ::EnterCriticalSection(&cs); }
    inline void Unlock()    { ::LeaveCriticalSection(&cs); }

#else
    pthread_mutex_t mutex;

public:
    static pthread_mutexattr_t RecursiveAttr;
    static bool                RecursiveAttrInit;

    Lock (unsigned spinCount = 0) // To do: Support spin count, probably via a custom lock implementation.
    {
        OVR_UNUSED(spinCount);
        if (!RecursiveAttrInit)
        {
            pthread_mutexattr_init(&RecursiveAttr);
            pthread_mutexattr_settype(&RecursiveAttr, PTHREAD_MUTEX_RECURSIVE);
            RecursiveAttrInit = 1;
        }
        pthread_mutex_init(&mutex,&RecursiveAttr);
    }
    ~Lock ()                { pthread_mutex_destroy(&mutex); }
    inline void DoLock()    { pthread_mutex_lock(&mutex); }
    inline void Unlock()    { pthread_mutex_unlock(&mutex); }

#endif // OVR_ENABLE_THREDS


public:
    // Locker class, used for automatic locking
    class Locker
    {
    public:     
        Lock *pLock;
        inline Locker(Lock *plock)
        { pLock = plock; pLock->DoLock(); }
        inline ~Locker()
        { pLock->Unlock();  }
    };
};


//-------------------------------------------------------------------------------------
// Globally shared Lock implementation used for MessageHandlers, etc.

class SharedLock
{    
public:
    SharedLock() : UseCount(0) {}

    Lock* GetLockAddRef();
    void  ReleaseLock(Lock* plock);
   
private:
    Lock* toLock() { return (Lock*)Buffer; }

    // UseCount and max alignment.
    AtomicInt<int>  UseCount;
    uint64_t        Buffer[(sizeof(Lock)+sizeof(uint64_t)-1)/sizeof(uint64_t)];
};


} // OVR

#endif
