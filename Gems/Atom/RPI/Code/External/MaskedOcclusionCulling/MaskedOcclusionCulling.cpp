////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <string.h>
#include <assert.h>
#include <float.h>
#include "MaskedOcclusionCulling.h"
#include "CompilerSpecific.inl"

#if MOC_RECORDER_ENABLE
#include "FrameRecorder.h"
#endif

#if defined(__AVX__) || defined(__AVX2__)
	// For performance reasons, the MaskedOcclusionCullingAVX2/512.cpp files should be compiled with VEX encoding for SSE instructions (to avoid 
	// AVX-SSE transition penalties, see https://software.intel.com/en-us/articles/avoiding-avx-sse-transition-penalties). However, this file
	// _must_ be compiled without VEX encoding to allow backwards compatibility. Best practice is to use lowest supported target platform 
	// (/arch:SSE2) as project default, and elevate only the MaskedOcclusionCullingAVX2/512.cpp files.
	#error The MaskedOcclusionCulling.cpp should be compiled with lowest supported target platform, e.g. /arch:SSE2
#endif

static MaskedOcclusionCulling::Implementation DetectCPUFeatures(MaskedOcclusionCulling::pfnAlignedAlloc alignedAlloc, MaskedOcclusionCulling::pfnAlignedFree alignedFree)
{
	struct CpuInfo { int regs[4]; };

	// Get regular CPUID values
	int regs[4];
	__cpuidex(regs, 0, 0);

    //  MOCVectorAllocator<CpuInfo> mocalloc( alignedAlloc, alignedFree );
    //  std::vector<CpuInfo, MOCVectorAllocator<CpuInfo>> cpuId( mocalloc ), cpuIdEx( mocalloc );
    //  cpuId.resize( regs[0] );
    size_t cpuIdCount = regs[0];
    CpuInfo * cpuId = (CpuInfo*)alignedAlloc( 64, sizeof(CpuInfo) * cpuIdCount );
    
	for (size_t i = 0; i < cpuIdCount; ++i)
		__cpuidex(cpuId[i].regs, (int)i, 0);

	// Get extended CPUID values
	__cpuidex(regs, 0x80000000, 0);

    //cpuIdEx.resize(regs[0] - 0x80000000);
    size_t cpuIdExCount = regs[0] - 0x80000000;
    CpuInfo * cpuIdEx = (CpuInfo*)alignedAlloc( 64, sizeof( CpuInfo ) * cpuIdExCount );

    for (size_t i = 0; i < cpuIdExCount; ++i)
		__cpuidex(cpuIdEx[i].regs, 0x80000000 + (int)i, 0);

	#define TEST_BITS(A, B)            (((A) & (B)) == (B))
	#define TEST_FMA_MOVE_OXSAVE       (cpuIdCount >= 1 && TEST_BITS(cpuId[1].regs[2], (1 << 12) | (1 << 22) | (1 << 27)))
	#define TEST_LZCNT                 (cpuIdExCount >= 1 && TEST_BITS(cpuIdEx[1].regs[2], 0x20))
	#define TEST_SSE41                 (cpuIdCount >= 1 && TEST_BITS(cpuId[1].regs[2], (1 << 19)))
	#define TEST_XMM_YMM               (cpuIdCount >= 1 && TEST_BITS(_xgetbv(0), (1 << 2) | (1 << 1)))
	#define TEST_OPMASK_ZMM            (cpuIdCount >= 1 && TEST_BITS(_xgetbv(0), (1 << 7) | (1 << 6) | (1 << 5)))
	#define TEST_BMI1_BMI2_AVX2        (cpuIdCount >= 7 && TEST_BITS(cpuId[7].regs[1], (1 << 3) | (1 << 5) | (1 << 8)))
	#define TEST_AVX512_F_BW_DQ        (cpuIdCount >= 7 && TEST_BITS(cpuId[7].regs[1], (1 << 16) | (1 << 17) | (1 << 30)))

    MaskedOcclusionCulling::Implementation retVal = MaskedOcclusionCulling::SSE2;
	if (TEST_FMA_MOVE_OXSAVE && TEST_LZCNT && TEST_SSE41)
	{
		if (TEST_XMM_YMM && TEST_OPMASK_ZMM && TEST_BMI1_BMI2_AVX2 && TEST_AVX512_F_BW_DQ)
			retVal = MaskedOcclusionCulling::AVX512;
		else if (TEST_XMM_YMM && TEST_BMI1_BMI2_AVX2)
			retVal = MaskedOcclusionCulling::AVX2;
	} 
    else if (TEST_SSE41)
		retVal = MaskedOcclusionCulling::SSE41;
    alignedFree( cpuId );
    alignedFree( cpuIdEx );
    return retVal;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions (not directly related to the algorithm/rasterizer)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MaskedOcclusionCulling::TransformVertices(const float *mtx, const float *inVtx, float *xfVtx, unsigned int nVtx, const VertexLayout &vtxLayout)
{
	// This function pretty slow, about 10-20% slower than if the vertices are stored in aligned SOA form.
	if (nVtx == 0)
		return;

	// Load matrix and swizzle out the z component. For post-multiplication (OGL), the matrix is assumed to be column 
	// major, with one column per SSE register. For pre-multiplication (DX), the matrix is assumed to be row major.
	__m128 mtxCol0 = _mm_loadu_ps(mtx);
	__m128 mtxCol1 = _mm_loadu_ps(mtx + 4);
	__m128 mtxCol2 = _mm_loadu_ps(mtx + 8);
	__m128 mtxCol3 = _mm_loadu_ps(mtx + 12);

	int stride = vtxLayout.mStride;
	const char *vPtr = (const char *)inVtx;
	float *outPtr = xfVtx;

	// Iterate through all vertices and transform
	for (unsigned int vtx = 0; vtx < nVtx; ++vtx)
	{
		__m128 xVal = _mm_load1_ps((float*)(vPtr));
		__m128 yVal = _mm_load1_ps((float*)(vPtr + vtxLayout.mOffsetY));
		__m128 zVal = _mm_load1_ps((float*)(vPtr + vtxLayout.mOffsetZ));

		__m128 xform = _mm_add_ps(_mm_mul_ps(mtxCol0, xVal), _mm_add_ps(_mm_mul_ps(mtxCol1, yVal), _mm_add_ps(_mm_mul_ps(mtxCol2, zVal), mtxCol3)));
		_mm_storeu_ps(outPtr, xform);
		vPtr += stride;
		outPtr += 4;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Typedefs
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef MaskedOcclusionCulling::pfnAlignedAlloc pfnAlignedAlloc;
typedef MaskedOcclusionCulling::pfnAlignedFree  pfnAlignedFree;
typedef MaskedOcclusionCulling::VertexLayout    VertexLayout;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common SSE2/SSE4.1 defines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIMD_LANES             4
#define TILE_HEIGHT_SHIFT      2

#define SIMD_LANE_IDX _mm_setr_epi32(0, 1, 2, 3)

#define SIMD_SUB_TILE_COL_OFFSET _mm_setr_epi32(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET _mm_setzero_si128()
#define SIMD_SUB_TILE_COL_OFFSET_F _mm_setr_ps(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET_F _mm_setzero_ps()

#define SIMD_LANE_YCOORD_I _mm_setr_epi32(128, 384, 640, 896)
#define SIMD_LANE_YCOORD_F _mm_setr_ps(128.0f, 384.0f, 640.0f, 896.0f)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common SSE2/SSE4.1 functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef __m128 __mw;
typedef __m128i __mwi;

#define _mmw_set1_ps                _mm_set1_ps
#define _mmw_setzero_ps             _mm_setzero_ps
#define _mmw_and_ps                 _mm_and_ps
#define _mmw_or_ps                  _mm_or_ps
#define _mmw_xor_ps                 _mm_xor_ps
#define _mmw_not_ps(a)              _mm_xor_ps((a), _mm_castsi128_ps(_mm_set1_epi32(~0)))
#define _mmw_andnot_ps              _mm_andnot_ps
#define _mmw_neg_ps(a)              _mm_xor_ps((a), _mm_set1_ps(-0.0f))
#define _mmw_abs_ps(a)              _mm_and_ps((a), _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)))
#define _mmw_add_ps                 _mm_add_ps
#define _mmw_sub_ps                 _mm_sub_ps
#define _mmw_mul_ps                 _mm_mul_ps
#define _mmw_div_ps                 _mm_div_ps
#define _mmw_min_ps                 _mm_min_ps
#define _mmw_max_ps                 _mm_max_ps
#define _mmw_movemask_ps            _mm_movemask_ps
#define _mmw_cmpge_ps(a,b)          _mm_cmpge_ps(a, b)
#define _mmw_cmpgt_ps(a,b)          _mm_cmpgt_ps(a, b)
#define _mmw_cmpeq_ps(a,b)          _mm_cmpeq_ps(a, b)
#define _mmw_fmadd_ps(a,b,c)        _mm_add_ps(_mm_mul_ps(a,b), c)
#define _mmw_fmsub_ps(a,b,c)        _mm_sub_ps(_mm_mul_ps(a,b), c)
#define _mmw_shuffle_ps             _mm_shuffle_ps
#define _mmw_insertf32x4_ps(a,b,c)  (b)
#define _mmw_cvtepi32_ps            _mm_cvtepi32_ps
#define _mmw_blendv_epi32(a,b,c)    simd_cast<__mwi>(_mmw_blendv_ps(simd_cast<__mw>(a), simd_cast<__mw>(b), simd_cast<__mw>(c)))

#define _mmw_set1_epi32             _mm_set1_epi32
#define _mmw_setzero_epi32          _mm_setzero_si128
#define _mmw_and_epi32              _mm_and_si128
#define _mmw_or_epi32               _mm_or_si128
#define _mmw_xor_epi32              _mm_xor_si128
#define _mmw_not_epi32(a)           _mm_xor_si128((a), _mm_set1_epi32(~0))
#define _mmw_andnot_epi32           _mm_andnot_si128
#define _mmw_neg_epi32(a)           _mm_sub_epi32(_mm_set1_epi32(0), (a))
#define _mmw_add_epi32              _mm_add_epi32
#define _mmw_sub_epi32              _mm_sub_epi32
#define _mmw_subs_epu16             _mm_subs_epu16
#define _mmw_cmpeq_epi32            _mm_cmpeq_epi32
#define _mmw_cmpgt_epi32            _mm_cmpgt_epi32
#define _mmw_srai_epi32             _mm_srai_epi32
#define _mmw_srli_epi32             _mm_srli_epi32
#define _mmw_slli_epi32             _mm_slli_epi32
#define _mmw_cvtps_epi32            _mm_cvtps_epi32
#define _mmw_cvttps_epi32           _mm_cvttps_epi32

#define _mmx_fmadd_ps               _mmw_fmadd_ps
#define _mmx_max_epi32              _mmw_max_epi32
#define _mmx_min_epi32              _mmw_min_epi32

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD casting functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, typename Y> FORCE_INLINE T simd_cast(Y A);
template<> FORCE_INLINE __m128  simd_cast<__m128>(float A) { return _mm_set1_ps(A); }
template<> FORCE_INLINE __m128  simd_cast<__m128>(__m128i A) { return _mm_castsi128_ps(A); }
template<> FORCE_INLINE __m128  simd_cast<__m128>(__m128 A) { return A; }
template<> FORCE_INLINE __m128i simd_cast<__m128i>(int A) { return _mm_set1_epi32(A); }
template<> FORCE_INLINE __m128i simd_cast<__m128i>(__m128 A) { return _mm_castps_si128(A); }
template<> FORCE_INLINE __m128i simd_cast<__m128i>(__m128i A) { return A; }

#define MAKE_ACCESSOR(name, simd_type, base_type, is_const, elements) \
	FORCE_INLINE is_const base_type * name(is_const simd_type &a) { \
		union accessor { simd_type m_native; base_type m_array[elements]; }; \
		is_const accessor *acs = reinterpret_cast<is_const accessor*>(&a); \
		return acs->m_array; \
	}

MAKE_ACCESSOR(simd_f32, __m128, float, , 4)
MAKE_ACCESSOR(simd_f32, __m128, float, const, 4)
MAKE_ACCESSOR(simd_i32, __m128i, int, , 4)
MAKE_ACCESSOR(simd_i32, __m128i, int, const, 4)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specialized SSE input assembly function for general vertex gather 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FORCE_INLINE void GatherVertices(__m128 *vtxX, __m128 *vtxY, __m128 *vtxW, const float *inVtx, const unsigned int *inTrisPtr, int numLanes, const VertexLayout &vtxLayout)
{
	for (int lane = 0; lane < numLanes; lane++)
	{
		for (int i = 0; i < 3; i++)
		{
			char *vPtrX = (char *)inVtx + inTrisPtr[lane * 3 + i] * vtxLayout.mStride;
			char *vPtrY = vPtrX + vtxLayout.mOffsetY;
			char *vPtrW = vPtrX + vtxLayout.mOffsetW;

			simd_f32(vtxX[i])[lane] = *((float*)vPtrX);
			simd_f32(vtxY[i])[lane] = *((float*)vPtrY);
			simd_f32(vtxW[i])[lane] = *((float*)vPtrW);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SSE4.1 version
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace MaskedOcclusionCullingSSE41
{
	FORCE_INLINE __m128i _mmw_mullo_epi32(const __m128i &a, const __m128i &b) { return _mm_mullo_epi32(a, b); }
	FORCE_INLINE __m128i _mmw_min_epi32(const __m128i &a, const __m128i &b) { return _mm_min_epi32(a, b); }
	FORCE_INLINE __m128i _mmw_max_epi32(const __m128i &a, const __m128i &b) { return _mm_max_epi32(a, b); }
	FORCE_INLINE __m128i _mmw_abs_epi32(const __m128i &a) { return _mm_abs_epi32(a); }
	FORCE_INLINE __m128 _mmw_blendv_ps(const __m128 &a, const __m128 &b, const __m128 &c) { return _mm_blendv_ps(a, b, c); }
	FORCE_INLINE int _mmw_testz_epi32(const __m128i &a, const __m128i &b) { return _mm_testz_si128(a, b); }
	FORCE_INLINE __m128 _mmx_dp4_ps(const __m128 &a, const __m128 &b) { return _mm_dp_ps(a, b, 0xFF); }
	FORCE_INLINE __m128 _mmw_floor_ps(const __m128 &a) { return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC); }
	FORCE_INLINE __m128 _mmw_ceil_ps(const __m128 &a) { return _mm_round_ps(a, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);	}
	FORCE_INLINE __m128i _mmw_transpose_epi8(const __m128i &a)
	{
		const __m128i shuff = _mm_setr_epi8(0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF);
		return _mm_shuffle_epi8(a, shuff);
	}
	FORCE_INLINE __m128i _mmw_sllv_ones(const __m128i &ishift)
	{
		__m128i shift = _mm_min_epi32(ishift, _mm_set1_epi32(32));

		// Uses lookup tables and _mm_shuffle_epi8 to perform _mm_sllv_epi32(~0, shift)
		const __m128i byteShiftLUT = _mm_setr_epi8((char)0xFF, (char)0xFE, (char)0xFC, (char)0xF8, (char)0xF0, (char)0xE0, (char)0xC0, (char)0x80, 0, 0, 0, 0, 0, 0, 0, 0);
		const __m128i byteShiftOffset = _mm_setr_epi8(0, 8, 16, 24, 0, 8, 16, 24, 0, 8, 16, 24, 0, 8, 16, 24);
		const __m128i byteShiftShuffle = _mm_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x4, 0x4, 0x4, 0x4, 0x8, 0x8, 0x8, 0x8, 0xC, 0xC, 0xC, 0xC);

		__m128i byteShift = _mm_shuffle_epi8(shift, byteShiftShuffle);
		byteShift = _mm_min_epi8(_mm_subs_epu8(byteShift, byteShiftOffset), _mm_set1_epi8(8));
		__m128i retMask = _mm_shuffle_epi8(byteShiftLUT, byteShift);

		return retMask;
	}

	static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::SSE41;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Include common algorithm implementation (general, SIMD independent code)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	#include "MaskedOcclusionCullingCommon.inl"

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utility function to create a new object using the allocator callbacks
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
	{
		MaskedOcclusionCullingPrivate *object = (MaskedOcclusionCullingPrivate *)alignedAlloc(64, sizeof(MaskedOcclusionCullingPrivate));
		new (object) MaskedOcclusionCullingPrivate(alignedAlloc, alignedFree);
		return object;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SSE2 version
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace MaskedOcclusionCullingSSE2
{
	FORCE_INLINE __m128i _mmw_mullo_epi32(const __m128i &a, const __m128i &b)
	{ 
		// Do products for even / odd lanes & merge the result
		__m128i even = _mm_and_si128(_mm_mul_epu32(a, b), _mm_setr_epi32(~0, 0, ~0, 0));
		__m128i odd = _mm_slli_epi64(_mm_mul_epu32(_mm_srli_epi64(a, 32), _mm_srli_epi64(b, 32)), 32);
		return _mm_or_si128(even, odd);
	}
	FORCE_INLINE __m128i _mmw_min_epi32(const __m128i &a, const __m128i &b)
	{ 
		__m128i cond = _mm_cmpgt_epi32(a, b);
		return _mm_or_si128(_mm_andnot_si128(cond, a), _mm_and_si128(cond, b));
	}
	FORCE_INLINE __m128i _mmw_max_epi32(const __m128i &a, const __m128i &b)
	{ 
		__m128i cond = _mm_cmpgt_epi32(b, a);
		return _mm_or_si128(_mm_andnot_si128(cond, a), _mm_and_si128(cond, b));
	}
	FORCE_INLINE __m128i _mmw_abs_epi32(const __m128i &a)
	{
		__m128i mask = _mm_cmplt_epi32(a, _mm_setzero_si128());
		return _mm_add_epi32(_mm_xor_si128(a, mask), _mm_srli_epi32(mask, 31));
	}
	FORCE_INLINE int _mmw_testz_epi32(const __m128i &a, const __m128i &b)
	{ 
		return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_and_si128(a, b), _mm_setzero_si128())) == 0xFFFF;
	}
	FORCE_INLINE __m128 _mmw_blendv_ps(const __m128 &a, const __m128 &b, const __m128 &c)
	{	
		__m128 cond = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(c), 31));
		return _mm_or_ps(_mm_andnot_ps(cond, a), _mm_and_ps(cond, b));
	}
	FORCE_INLINE __m128 _mmx_dp4_ps(const __m128 &a, const __m128 &b)
	{ 
		// Product and two shuffle/adds pairs (similar to hadd_ps)
		__m128 prod = _mm_mul_ps(a, b);
		__m128 dp = _mm_add_ps(prod, _mm_shuffle_ps(prod, prod, _MM_SHUFFLE(2, 3, 0, 1)));
		dp = _mm_add_ps(dp, _mm_shuffle_ps(dp, dp, _MM_SHUFFLE(0, 1, 2, 3)));
		return dp;
	}
	FORCE_INLINE __m128 _mmw_floor_ps(const __m128 &a)
	{ 
		int originalMode = _MM_GET_ROUNDING_MODE();
		_MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
		__m128 rounded = _mm_cvtepi32_ps(_mm_cvtps_epi32(a));
		_MM_SET_ROUNDING_MODE(originalMode);
		return rounded;
	}
	FORCE_INLINE __m128 _mmw_ceil_ps(const __m128 &a)
	{ 
		int originalMode = _MM_GET_ROUNDING_MODE();
		_MM_SET_ROUNDING_MODE(_MM_ROUND_UP);
		__m128 rounded = _mm_cvtepi32_ps(_mm_cvtps_epi32(a));
		_MM_SET_ROUNDING_MODE(originalMode);
		return rounded;
	}
	FORCE_INLINE __m128i _mmw_transpose_epi8(const __m128i &a)
	{
		// Perform transpose through two 16->8 bit pack and byte shifts
		__m128i res = a;
		const __m128i mask = _mm_setr_epi8(~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0);
		res = _mm_packus_epi16(_mm_and_si128(res, mask), _mm_srli_epi16(res, 8));
		res = _mm_packus_epi16(_mm_and_si128(res, mask), _mm_srli_epi16(res, 8));
		return res;
	}
	FORCE_INLINE __m128i _mmw_sllv_ones(const __m128i &ishift)
	{
		__m128i shift = _mmw_min_epi32(ishift, _mm_set1_epi32(32));
		
		// Uses scalar approach to perform _mm_sllv_epi32(~0, shift)
		static const unsigned int maskLUT[33] = {
			~0U << 0, ~0U << 1, ~0U << 2 ,  ~0U << 3, ~0U << 4, ~0U << 5, ~0U << 6 , ~0U << 7, ~0U << 8, ~0U << 9, ~0U << 10 , ~0U << 11, ~0U << 12, ~0U << 13, ~0U << 14 , ~0U << 15,
			~0U << 16, ~0U << 17, ~0U << 18 , ~0U << 19, ~0U << 20, ~0U << 21, ~0U << 22 , ~0U << 23, ~0U << 24, ~0U << 25, ~0U << 26 , ~0U << 27, ~0U << 28, ~0U << 29, ~0U << 30 , ~0U << 31,
			0U };

		__m128i retMask;
		simd_i32(retMask)[0] = (int)maskLUT[simd_i32(shift)[0]];
		simd_i32(retMask)[1] = (int)maskLUT[simd_i32(shift)[1]];
		simd_i32(retMask)[2] = (int)maskLUT[simd_i32(shift)[2]];
		simd_i32(retMask)[3] = (int)maskLUT[simd_i32(shift)[3]];
		return retMask;
	}

	static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::SSE2;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Include common algorithm implementation (general, SIMD independent code)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	#include "MaskedOcclusionCullingCommon.inl"

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utility function to create a new object using the allocator callbacks
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
	{
		MaskedOcclusionCullingPrivate *object = (MaskedOcclusionCullingPrivate *)alignedAlloc(64, sizeof(MaskedOcclusionCullingPrivate));
		new (object) MaskedOcclusionCullingPrivate(alignedAlloc, alignedFree);
		return object;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Object construction and allocation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace MaskedOcclusionCullingAVX512
{
	extern MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree);
}

namespace MaskedOcclusionCullingAVX2
{
	extern MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree);
}

MaskedOcclusionCulling *MaskedOcclusionCulling::Create(Implementation RequestedSIMD)
{
	return Create(RequestedSIMD, aligned_alloc, aligned_free);
}

MaskedOcclusionCulling *MaskedOcclusionCulling::Create(Implementation RequestedSIMD, pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
{
	MaskedOcclusionCulling *object = nullptr;

	MaskedOcclusionCulling::Implementation impl = DetectCPUFeatures(alignedAlloc, alignedFree);

	if (RequestedSIMD < impl)
		impl = RequestedSIMD;

	// Return best supported version
	if (object == nullptr && impl >= AVX512)
		object = MaskedOcclusionCullingAVX512::CreateMaskedOcclusionCulling(alignedAlloc, alignedFree); // Use AVX512 version
	if (object == nullptr && impl >= AVX2)
		object = MaskedOcclusionCullingAVX2::CreateMaskedOcclusionCulling(alignedAlloc, alignedFree); // Use AVX2 version
	if (object == nullptr && impl >= SSE41)
		object = MaskedOcclusionCullingSSE41::CreateMaskedOcclusionCulling(alignedAlloc, alignedFree); // Use SSE4.1 version
	if (object == nullptr)
		object = MaskedOcclusionCullingSSE2::CreateMaskedOcclusionCulling(alignedAlloc, alignedFree); // Use SSE2 (slow) version

	return object;
}

void MaskedOcclusionCulling::Destroy(MaskedOcclusionCulling *moc)
{
	pfnAlignedFree alignedFreeCallback = moc->mAlignedFreeCallback;
	moc->~MaskedOcclusionCulling();
	alignedFreeCallback(moc);
}
