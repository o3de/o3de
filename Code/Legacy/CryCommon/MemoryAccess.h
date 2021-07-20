/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Misc mathematical functions

#pragma once


#include <platform.h>

// Section dictionary
#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define MEMORYACCESS_H_SECTION_TRAITS 1
#define MEMORYACCESS_H_SECTION_CRYPREFETCH 2
#endif

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION MEMORYACCESS_H_SECTION_TRAITS
    #include AZ_RESTRICTED_FILE(MemoryAccess_h)
#else
#define MEMORYACCESS_H_TRAIT_USE_LEGACY_PREFETCHLINE 1
#endif

#if MEMORYACCESS_H_TRAIT_USE_LEGACY_PREFETCHLINE
#define PrefetchLine(ptr, off) cryPrefetchT0SSE((void*)((UINT_PTR)ptr + off))
#else
#define PrefetchLine(ptr, off) (void)(0)
#endif
#define ResetLine128(ptr, off) (void)(0)
#define FlushLine128(ptr, off) (void)(0)



//========================================================================================

// cryMemcpy flags
#define MC_CPU_TO_GPU 0x10
#define MC_GPU_TO_CPU 0x20
#define MC_CPU_TO_CPU 0x40

extern int g_CpuFlags;

//
#define CPUF_SSE   0x01
#define CPUF_SSE2  0x02
#define CPUF_3DNOW 0x04
#define CPUF_MMX   0x08
#define CPUF_SSE3  0x10
#define CPUF_F16C  0x20
#define CPUF_SSE41 0x40

#ifdef _CPU_SSE

#ifdef _CPU_X86
#include <xmmintrin.h>
#endif

#define _MM_PREFETCH(MemPtr, Hint) _mm_prefetch((MemPtr), (Hint));
#define _MM_PREFETCH_LOOP(nCount, MemPtr, Hint) { for (int p = 0; p < nCount; p += 64) { _mm_prefetch((const char*)(MemPtr) + p, Hint); } \
}
#else //_CPU_SSE
#define _MM_PREFETCH(MemPtr, Hint)
#define _MM_PREFETCH_LOOP(nCount, MemPtr, Hint)
#endif //_CPU_SSE

void cryMemcpy(void* Dst, const void* Src, int Count);
#if defined(LINUX) || defined(APPLE)
// Define this for Mac and Linux since it is used with the pthread sources
  #define mymemcpy16 memcpy
#endif


//==========================================================================================
// 3DNow! optimizations

#if defined _CPU_X86 && !defined(LINUX) && !defined(APPLE)
// ***************************************************************************
inline void cryPrecacheSSE(const void* src, int nbytes)
{
    _asm
    {
        mov esi, src
        mov ecx, nbytes
        // 64 bytes per pass
        shr ecx, 6
        jz endLabel

loopMemToL1:
        prefetchnta 64[ESI] // Prefetch next loop, non-temporal
        prefetchnta 96[ESI]

        movq mm1,  0[ESI]// Read in source data
        movq mm2,  8[ESI]
        movq mm3, 16[ESI]
        movq mm4, 24[ESI]
        movq mm5, 32[ESI]
        movq mm6, 40[ESI]
        movq mm7, 48[ESI]
        movq mm0, 56[ESI]

        add esi, 64
        dec ecx
        jnz loopMemToL1

        emms

endLabel:
    }
}


#endif



ILINE void cryPrefetchT0SSE(const void* src)
{
#if defined(WIN32) && !defined(WIN64)
    _asm
    {
        mov esi, src
            prefetchT0 [ESI] // Prefetch
    }
#else
    _MM_PREFETCH((char*)src, _MM_HINT_T0);
#endif
}

//=================================================================================

// Very optimized memcpy() routine for AMD Athlon and Duron family.
// This code uses any of FOUR different basic copy methods, depending
// on the transfer size.
// NOTE:  Since this code uses MOVNTQ (also known as "Non-Temporal MOV" or
// "Streaming Store"), and also uses the software prefetch instructions,
// be sure you're running on Athlon/Duron or other recent CPU before calling!

#define TINY_BLOCK_COPY 64       // Upper limit for movsd type copy.
// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".

#define IN_CACHE_COPY 64 * 1024  // Upper limit for movq/movq copy w/SW prefetch.
// Next is a copy that uses the MMX registers to copy 8 bytes at a time,
// also using the "unrolled loop" optimization.   This code uses
// the software prefetch instruction to get the data into the cache.

#define UNCACHED_COPY 197 * 1024 // Upper limit for movq/movntq w/SW prefetch.
// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
// USE 64 * 1024 FOR THIS VALUE IF YOU'RE ALWAYS FILLING A "CLEAN CACHE".

#define BLOCK_PREFETCH_COPY  infinity // No limit for movq/movntq w/block prefetch.
#define CACHEBLOCK 80h // Number of 64-byte blocks (cache lines) for block prefetch.
// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch.  The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.


#if defined _CPU_X86 && !defined(LINUX) && !defined(APPLE)
// Inline assembly syntax for use with Visual C++
inline void cryMemcpy(void* Dst, const void* Src, int Count)
{
    if (g_CpuFlags & CPUF_SSE)
    {
        __asm
        {
            mov       ecx, [Count];
            number of bytes to copy
            mov       edi, [Dst];
            destination
            mov       esi, [Src];
            source
            mov       ebx, ecx;
            keep a copy of count

            cld
            cmp       ecx, TINY_BLOCK_COPY
            jb        $memcpy_ic_3;
            tiny ? skip mmx copy

            cmp       ecx, 32 * 1024;
            dont align between 32k - 64k because
            jbe       $memcpy_do_align;
            it appears to be slower
            cmp       ecx, 64*1024
            jbe       $memcpy_align_done
                      $memcpy_do_align :
            mov       ecx, 8;
            a trick thats faster than rep movsb ...
            sub       ecx, edi;
            align destination to qword
            and       ecx, 111b;
            get the low bits
            sub       ebx, ecx;
            update copy count
            neg       ecx;
            set up to jump into the array
            add       ecx, offset $memcpy_align_done
            jmp       ecx;
            jump to array of movsbs

                align 4
            movsb
            movsb
            movsb
            movsb
            movsb
            movsb
            movsb
            movsb

$memcpy_align_done:;
            destination is dword aligned
            mov       ecx, ebx;
            number of bytes left to copy
            shr       ecx, 6;
            get 64 - byte block count
            jz        $memcpy_ic_2;
            finish the last few bytes

            cmp       ecx, IN_CACHE_COPY / 64;
            too big 4 cache ? use uncached copy
            jae       $memcpy_uc_test

            // This is small block copy that uses the MMX registers to copy 8 bytes
            // at a time.  It uses the "unrolled loop" optimization, and also uses
            // the software prefetch instruction to get the data into the cache.
                align 16
            $memcpy_ic_1 :;
            64 - byte block copies, in - cache copy

            prefetchnta [esi + (200 * 64 / 34 + 192)];
            start reading ahead

            movq  mm0, [esi + 0];
            read 64 bits
            movq  mm1, [esi + 8]
            movq  [edi + 0], mm0;
            write 64 bits
            movq  [edi + 8], mm1;
note:  the normal movq writes the
            movq  mm2, [esi + 16];
            data to cache;
            a cache line will be
            movq  mm3, [esi + 24];
            allocated as needed, to store the data
                         movq  [edi + 16], mm2
                         movq  [edi + 24], mm3
            movq  mm0, [esi + 32]
            movq  mm1, [esi + 40]
            movq  [edi + 32], mm0
                         movq  [edi + 40], mm1
            movq  mm2, [esi + 48]
            movq  mm3, [esi + 56]
            movq  [edi + 48], mm2
                         movq  [edi + 56], mm3

            add       esi, 64;
            update source pointer
            add       edi, 64;
            update destination pointer
            dec       ecx;
            count down
            jnz       $memcpy_ic_1;
            last 64 - byte block ?

            $memcpy_ic_2 :
            mov       ecx, ebx;
            has valid low 6 bits of the byte count
$memcpy_ic_3:
            shr       ecx, 2;
            dword count
            and       ecx, 1111b;
            only look at the "remainder" bits
            neg       ecx;
            set up to jump into the array
            add       ecx, offset $memcpy_last_few
            jmp       ecx;
            jump to array of movsds

$memcpy_uc_test:
            cmp       ecx, UNCACHED_COPY / 64;
            big enough ? use block prefetch copy
            jae       $memcpy_bp_1

                $memcpy_64_test :
            or        ecx, ecx;
            tail end of block prefetch will jump here
            jz        $memcpy_ic_2;
            no more 64 - byte blocks left

            // For larger blocks, which will spill beyond the cache, it's faster to
            // use the Streaming Store instruction MOVNTQ.   This write instruction
            // bypasses the cache and writes straight to main memory.  This code also
            // uses the software prefetch instruction to pre-read the data.
               align 16
$memcpy_uc_1:;
            64 - byte blocks, uncached copy

            prefetchnta [esi + (200 * 64 / 34 + 192)];
            start reading ahead

            movq  mm0, [esi + 0];
            read 64 bits
            add       edi, 64;
            update destination pointer
            movq  mm1, [esi + 8]
            add       esi, 64;
            update source pointer
            movq  mm2, [esi - 48]
            movntq    [edi - 64], mm0;
            write 64 bits, bypassing the cache
            movq  mm0, [esi - 40];
note: movntq also prevents the CPU
            movntq    [edi - 56], mm1;
            from READING the destination address
            movq  mm1, [esi - 32];
            into the cache, only to be over - written
                     movntq    [edi - 48], mm2;
            so that also helps performance
            movq  mm2, [esi - 24]
            movntq    [edi - 40], mm0
            movq  mm0, [esi - 16]
            movntq    [edi - 32], mm1
            movq  mm1, [esi - 8]
            movntq    [edi - 24], mm2
                  movntq    [edi - 16], mm0
            dec       ecx
                  movntq    [edi - 8], mm1
            jnz       $memcpy_uc_1;
            last 64 - byte block ?

            jmp       $memcpy_ic_2;
            almost done

            // For the largest size blocks, a special technique called Block Prefetch
            // can be used to accelerate the read operations.   Block Prefetch reads
            // one address per cache line, for a series of cache lines, in a short loop.
            // This is faster than using software prefetch.  The technique is great for
            // getting maximum read bandwidth, especially in DDR memory systems.
                $memcpy_bp_1 :;
            large blocks, block prefetch copy

            cmp       ecx, CACHEBLOCK;
            big enough to run another prefetch loop ?
            jl        $memcpy_64_test;
            no, back to regular uncached copy

            mov       eax, CACHEBLOCK / 2;
            block prefetch loop, unrolled 2X
            add       esi, CACHEBLOCK* 64;
            move to the top of the block
                align 16
            $memcpy_bp_2 :
            mov       edx, [esi - 64];
            grab one address per cache line
            mov       edx, [esi - 128];
            grab one address per cache line
            sub       esi, 128;
            go reverse order to suppress HW prefetcher
            dec       eax;
            count down the cache lines
            jnz       $memcpy_bp_2;
            keep grabbing more lines into cache

            mov       eax, CACHEBLOCK;
            now that its in cache, do
            {
                the copy
                            align 16
$memcpy_bp_3:
                movq  mm0, [esi   ];
            } read 64 bits
            movq  mm1, [esi + 8]
            movq  mm2, [esi + 16]
            movq  mm3, [esi + 24]
            movq  mm4, [esi + 32]
            movq  mm5, [esi + 40]
            movq  mm6, [esi + 48]
            movq  mm7, [esi + 56]
            add       esi, 64;
            update source pointer
                movntq    [edi   ], mm0;
            write 64 bits, bypassing cache
            movntq    [edi + 8], mm1;
note: movntq also prevents the CPU
            movntq    [edi + 16], mm2;
            from READING the destination address
                movntq    [edi + 24], mm3;
            into the cache, only to be over - written,
                     movntq    [edi + 32], mm4;
            so that also helps performance
                movntq    [edi + 40], mm5
                movntq    [edi + 48], mm6
                movntq    [edi + 56], mm7
            add       edi, 64;
            update dest pointer

            dec       eax;
            count down

            jnz       $memcpy_bp_3;
            keep copying
            sub       ecx, CACHEBLOCK;
            update the 64 - byte block count
            jmp       $memcpy_bp_1;
            keep processing chunks

            // The smallest copy uses the X86 "movsd" instruction, in an optimized
            // form which is an "unrolled loop".   Then it handles the last few bytes.
                align 4
            movsd
                movsd;
            perform last 1 - 15 dword copies
            movsd
            movsd
            movsd
            movsd
            movsd
            movsd
            movsd
                    movsd;
            perform last 1 - 7 dword copies
            movsd
            movsd
            movsd
            movsd
            movsd
            movsd

$memcpy_last_few:;
            dword aligned from before movsds
            mov       ecx, ebx;
            has valid low 2 bits of the byte count
            and       ecx, 11b;
            the last few cows must come home
            jz        $memcpy_final;
            no more, lets leave
            rep       movsb;
            the last 1, 2, or 3 bytes

$memcpy_final:
            emms;
            clean up the MMX state
                sfence;
            flush the write buffer
            //  mov     eax, [dest] ; ret value = destination pointer
        }
    }
    else
    {
        memcpy(Dst, Src, Count);
    }
}

inline void cryPrefetch(const void* Src, int nCount)
{
    nCount >>= 6;
    if (nCount > 0)
    {
        _asm
        {
            mov esi, Src;
            mov ecx, nCount;
mPr0:
            align 16
            dec ecx;
            mov eax, [esi];
            mov eax, 0;
            lea esi, [esi + 40h];
            jne mPr0;
        }
    }
    else
    {
        _asm
        {
            mov esi, Src;
            mov ecx, nCount;
mPr1:
            align 16
            inc ecx;
            mov eax, [esi];
            mov eax, 0;
            lea esi, [esi - 40h];
            jne mPr1;
        }
    }
}

inline void cryMemcpy (void* inDst, const void* inSrc, int nCount, int nFlags)
{
    cryMemcpy(inDst, inSrc, nCount);
}

//==========================================================================================
// SSE optimizations


#else

const int PREFNTA_BLOCK = 0x4000;

ILINE void cryMemcpy(void* Dst, const void* Src, int n)
{
    char* dst = (char*)Dst;
    char* src = (char*)Src;
    while (n > PREFNTA_BLOCK)
    {
        _MM_PREFETCH_LOOP(PREFNTA_BLOCK, src, _MM_HINT_NTA);

        memcpy(dst, src, PREFNTA_BLOCK);
        src += PREFNTA_BLOCK;
        dst += PREFNTA_BLOCK;
        n -= PREFNTA_BLOCK;
    }
    _MM_PREFETCH_LOOP(n, src, _MM_HINT_NTA);
    memcpy(dst, src, n);
}

ILINE void cryMemcpy(void* Dst, const void* Src, int n, [[maybe_unused]] int nFlags)
{
    char* dst = (char*)Dst;
    char* src = (char*)Src;
    while (n > PREFNTA_BLOCK)
    {
        _MM_PREFETCH_LOOP(PREFNTA_BLOCK, src, _MM_HINT_NTA);
        memcpy(dst, src, PREFNTA_BLOCK);
        src += PREFNTA_BLOCK;
        dst += PREFNTA_BLOCK;
        n -= PREFNTA_BLOCK;
    }
    _MM_PREFETCH_LOOP(n, src, _MM_HINT_NTA);
    memcpy(dst, src, n);
}


#endif


#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION MEMORYACCESS_H_SECTION_CRYPREFETCH
    #include AZ_RESTRICTED_FILE(MemoryAccess_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
//implement something usual to bring one memory location into L1 data cache
ILINE void CryPrefetch(const void* const cpSrc)
{
    cryPrefetchT0SSE(cpSrc);
}
#endif

#define CryPrefetchInl CryPrefetch
