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
#include <string.h>
#include <assert.h>
#include <float.h>
#include "MaskedOcclusionCulling.h"
#include "CompilerSpecific.inl"

#if MOC_RECORDER_ENABLE
#include "FrameRecorder.h"
#endif

// Make sure compiler supports AVX-512 intrinsics: Visual Studio 2017 (Update 3) || Intel C++ Compiler 16.0 || Clang 4.0 || GCC 5.0
#if USE_AVX512 != 0 && ((defined(_MSC_VER) && _MSC_VER >= 1911) || (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 1600) || (defined(__clang__) && __clang_major__ >= 4) || (defined(__GNUC__) && __GNUC__ >= 5))

// The MaskedOcclusionCullingAVX512.cpp file should be compiled avx2/avx512 architecture options turned on in the compiler. However, the SSE
// version in MaskedOcclusionCulling.cpp _must_ be compiled with SSE2 architecture allow backwards compatibility. Best practice is to 
// use lowest supported target platform (e.g. /arch:SSE2) as project default, and elevate only the MaskedOcclusionCullingAVX2/512.cpp files.
#ifndef __AVX2__
	#error For best performance, MaskedOcclusionCullingAVX512.cpp should be compiled with /arch:AVX2
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AVX specific defines and constants
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIMD_LANES             16
#define TILE_HEIGHT_SHIFT      4

#define SIMD_LANE_IDX _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)

#define SIMD_SUB_TILE_COL_OFFSET _mm512_setr_epi32(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET _mm512_setr_epi32(0, 0, 0, 0, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT * 2, SUB_TILE_HEIGHT * 2, SUB_TILE_HEIGHT * 2, SUB_TILE_HEIGHT * 2, SUB_TILE_HEIGHT * 3, SUB_TILE_HEIGHT * 3, SUB_TILE_HEIGHT * 3, SUB_TILE_HEIGHT * 3)
#define SIMD_SUB_TILE_COL_OFFSET_F _mm512_setr_ps(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET_F _mm512_setr_ps(0, 0, 0, 0, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT * 2, SUB_TILE_HEIGHT * 2, SUB_TILE_HEIGHT * 2, SUB_TILE_HEIGHT * 2, SUB_TILE_HEIGHT * 3, SUB_TILE_HEIGHT * 3, SUB_TILE_HEIGHT * 3, SUB_TILE_HEIGHT * 3)

#define SIMD_SHUFFLE_SCANLINE_TO_SUBTILES _mm512_set_epi32(0x0F0B0703, 0x0E0A0602, 0x0D090501, 0x0C080400, 0x0F0B0703, 0x0E0A0602, 0x0D090501, 0x0C080400, 0x0F0B0703, 0x0E0A0602, 0x0D090501, 0x0C080400, 0x0F0B0703, 0x0E0A0602, 0x0D090501, 0x0C080400)

#define SIMD_LANE_YCOORD_I _mm512_setr_epi32(128, 384, 640, 896, 1152, 1408, 1664, 1920, 2176, 2432, 2688, 2944, 3200, 3456, 3712, 3968)
#define SIMD_LANE_YCOORD_F _mm512_setr_ps(128.0f, 384.0f, 640.0f, 896.0f, 1152.0f, 1408.0f, 1664.0f, 1920.0f, 2176.0f, 2432.0f, 2688.0f, 2944.0f, 3200.0f, 3456.0f, 3712.0f, 3968.0f)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AVX specific typedefs and functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef __m512 __mw;
typedef __m512i __mwi;

#define _mmw_set1_ps                _mm512_set1_ps
#define _mmw_setzero_ps             _mm512_setzero_ps
#define _mmw_and_ps                 _mm512_and_ps
#define _mmw_or_ps                  _mm512_or_ps
#define _mmw_xor_ps                 _mm512_xor_ps
#define _mmw_not_ps(a)              _mm512_xor_ps((a), _mm512_castsi512_ps(_mm512_set1_epi32(~0)))
#define _mmw_andnot_ps              _mm512_andnot_ps
#define _mmw_neg_ps(a)              _mm512_xor_ps((a), _mm512_set1_ps(-0.0f))
#define _mmw_abs_ps(a)              _mm512_and_ps((a), _mm512_castsi512_ps(_mm512_set1_epi32(0x7FFFFFFF)))
#define _mmw_add_ps                 _mm512_add_ps
#define _mmw_sub_ps                 _mm512_sub_ps
#define _mmw_mul_ps                 _mm512_mul_ps
#define _mmw_div_ps                 _mm512_div_ps
#define _mmw_min_ps                 _mm512_min_ps
#define _mmw_max_ps                 _mm512_max_ps
#define _mmw_fmadd_ps               _mm512_fmadd_ps
#define _mmw_fmsub_ps               _mm512_fmsub_ps
#define _mmw_shuffle_ps             _mm512_shuffle_ps
#define _mmw_insertf32x4_ps         _mm512_insertf32x4
#define _mmw_cvtepi32_ps            _mm512_cvtepi32_ps
#define _mmw_blendv_epi32(a,b,c)    simd_cast<__mwi>(_mmw_blendv_ps(simd_cast<__mw>(a), simd_cast<__mw>(b), simd_cast<__mw>(c)))

#define _mmw_set1_epi32             _mm512_set1_epi32
#define _mmw_setzero_epi32          _mm512_setzero_si512
#define _mmw_and_epi32              _mm512_and_si512
#define _mmw_or_epi32               _mm512_or_si512
#define _mmw_xor_epi32              _mm512_xor_si512
#define _mmw_not_epi32(a)           _mm512_xor_si512((a), _mm512_set1_epi32(~0))
#define _mmw_andnot_epi32           _mm512_andnot_si512
#define _mmw_neg_epi32(a)           _mm512_sub_epi32(_mm512_set1_epi32(0), (a))
#define _mmw_add_epi32              _mm512_add_epi32
#define _mmw_sub_epi32              _mm512_sub_epi32
#define _mmw_min_epi32              _mm512_min_epi32
#define _mmw_max_epi32              _mm512_max_epi32
#define _mmw_subs_epu16             _mm512_subs_epu16
#define _mmw_mullo_epi32            _mm512_mullo_epi32
#define _mmw_srai_epi32             _mm512_srai_epi32
#define _mmw_srli_epi32             _mm512_srli_epi32
#define _mmw_slli_epi32             _mm512_slli_epi32
#define _mmw_sllv_ones(x)           _mm512_sllv_epi32(SIMD_BITS_ONE, x)
#define _mmw_transpose_epi8(x)      _mm512_shuffle_epi8(x, SIMD_SHUFFLE_SCANLINE_TO_SUBTILES)
#define _mmw_abs_epi32              _mm512_abs_epi32
#define _mmw_cvtps_epi32            _mm512_cvtps_epi32
#define _mmw_cvttps_epi32           _mm512_cvttps_epi32

#define _mmx_dp4_ps(a, b)           _mm_dp_ps(a, b, 0xFF)
#define _mmx_fmadd_ps               _mm_fmadd_ps
#define _mmx_max_epi32              _mm_max_epi32
#define _mmx_min_epi32              _mm_min_epi32

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
template<> FORCE_INLINE __m256  simd_cast<__m256>(float A) { return _mm256_set1_ps(A); }
template<> FORCE_INLINE __m256  simd_cast<__m256>(__m256i A) { return _mm256_castsi256_ps(A); }
template<> FORCE_INLINE __m256  simd_cast<__m256>(__m256 A) { return A; }
template<> FORCE_INLINE __m256i simd_cast<__m256i>(int A) { return _mm256_set1_epi32(A); }
template<> FORCE_INLINE __m256i simd_cast<__m256i>(__m256 A) { return _mm256_castps_si256(A); }
template<> FORCE_INLINE __m256i simd_cast<__m256i>(__m256i A) { return A; }
template<> FORCE_INLINE __m512  simd_cast<__m512>(float A) { return _mm512_set1_ps(A); }
template<> FORCE_INLINE __m512  simd_cast<__m512>(__m512i A) { return _mm512_castsi512_ps(A); }
template<> FORCE_INLINE __m512  simd_cast<__m512>(__m512 A) { return A; }
template<> FORCE_INLINE __m512i simd_cast<__m512i>(int A) { return _mm512_set1_epi32(A); }
template<> FORCE_INLINE __m512i simd_cast<__m512i>(__m512 A) { return _mm512_castps_si512(A); }
template<> FORCE_INLINE __m512i simd_cast<__m512i>(__m512i A) { return A; }

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

MAKE_ACCESSOR(simd_f32, __m256, float, , 8)
MAKE_ACCESSOR(simd_f32, __m256, float, const, 8)
MAKE_ACCESSOR(simd_i32, __m256i, int, , 8)
MAKE_ACCESSOR(simd_i32, __m256i, int, const, 8)

MAKE_ACCESSOR(simd_f32, __m512, float, , 16)
MAKE_ACCESSOR(simd_f32, __m512, float, const, 16)
MAKE_ACCESSOR(simd_i32, __m512i, int, , 16)
MAKE_ACCESSOR(simd_i32, __m512i, int, const, 16)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specialized AVX input assembly function for general vertex gather 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef MaskedOcclusionCulling::VertexLayout VertexLayout;

FORCE_INLINE void GatherVertices(__m512 *vtxX, __m512 *vtxY, __m512 *vtxW, const float *inVtx, const unsigned int *inTrisPtr, int numLanes, const VertexLayout &vtxLayout)
{
	assert(numLanes >= 1);

	const __m512i SIMD_TRI_IDX_OFFSET = _mm512_setr_epi32(0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45);
	static const __m512i SIMD_LANE_MASK[17] = {
		_mm512_setr_epi32( 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0,  0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,  0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,  0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,  0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,  0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,  0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,  0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,  0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,  0),
		_mm512_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0)
	};

	// Compute per-lane index list offset that guards against out of bounds memory accesses
	__m512i safeTriIdxOffset = _mm512_and_si512(SIMD_TRI_IDX_OFFSET, SIMD_LANE_MASK[numLanes]);

	// Fetch triangle indices. 
	__m512i vtxIdx[3];
	vtxIdx[0] = _mmw_mullo_epi32(_mm512_i32gather_epi32(safeTriIdxOffset, (const int*)inTrisPtr + 0, 4), _mmw_set1_epi32(vtxLayout.mStride));
	vtxIdx[1] = _mmw_mullo_epi32(_mm512_i32gather_epi32(safeTriIdxOffset, (const int*)inTrisPtr + 1, 4), _mmw_set1_epi32(vtxLayout.mStride));
	vtxIdx[2] = _mmw_mullo_epi32(_mm512_i32gather_epi32(safeTriIdxOffset, (const int*)inTrisPtr + 2, 4), _mmw_set1_epi32(vtxLayout.mStride));

	char *vPtr = (char *)inVtx;

	// Fetch triangle vertices
	for (int i = 0; i < 3; i++)
	{
		vtxX[i] = _mm512_i32gather_ps(vtxIdx[i], (float *)vPtr, 1);
		vtxY[i] = _mm512_i32gather_ps(vtxIdx[i], (float *)(vPtr + vtxLayout.mOffsetY), 1);
		vtxW[i] = _mm512_i32gather_ps(vtxIdx[i], (float *)(vPtr + vtxLayout.mOffsetW), 1);
	}
}

namespace MaskedOcclusionCullingAVX512
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Poorly implemented functions. TODO: fix common (maskedOcclusionCullingCommon.inl) code to improve perf
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	FORCE_INLINE __m512 _mmw_floor_ps(__m512 x)
	{
		return _mm512_roundscale_ps(x, 1); // 1 = floor
	}

	FORCE_INLINE __m512 _mmw_ceil_ps(__m512 x)
	{
		return _mm512_roundscale_ps(x, 2); // 2 = ceil
	}

	FORCE_INLINE __m512i _mmw_cmpeq_epi32(__m512i a, __m512i b)
	{
		__mmask16 mask = _mm512_cmpeq_epi32_mask(a, b);
		return _mm512_mask_mov_epi32(_mm512_set1_epi32(0), mask, _mm512_set1_epi32(~0));
	}

	FORCE_INLINE __m512i _mmw_cmpgt_epi32(__m512i a, __m512i b)
	{
		__mmask16 mask = _mm512_cmpgt_epi32_mask(a, b);
		return _mm512_mask_mov_epi32(_mm512_set1_epi32(0), mask, _mm512_set1_epi32(~0));
	}

	FORCE_INLINE bool _mmw_testz_epi32(__m512i a, __m512i b)
	{
		__mmask16 mask = _mm512_cmpeq_epi32_mask(_mm512_and_si512(a, b), _mm512_set1_epi32(0));
		return mask == 0xFFFF;
	}

	FORCE_INLINE __m512 _mmw_cmpge_ps(__m512 a, __m512 b)
	{
		__mmask16 mask = _mm512_cmp_ps_mask(a, b, _CMP_GE_OQ);
		return _mm512_castsi512_ps(_mm512_mask_mov_epi32(_mm512_set1_epi32(0), mask, _mm512_set1_epi32(~0)));
	}

	FORCE_INLINE __m512 _mmw_cmpgt_ps(__m512 a, __m512 b)
	{
		__mmask16 mask = _mm512_cmp_ps_mask(a, b, _CMP_GT_OQ);
		return _mm512_castsi512_ps(_mm512_mask_mov_epi32(_mm512_set1_epi32(0), mask, _mm512_set1_epi32(~0)));
	}

	FORCE_INLINE __m512 _mmw_cmpeq_ps(__m512 a, __m512 b)
	{
		__mmask16 mask = _mm512_cmp_ps_mask(a, b, _CMP_EQ_OQ);
		return _mm512_castsi512_ps(_mm512_mask_mov_epi32(_mm512_set1_epi32(0), mask, _mm512_set1_epi32(~0)));
	}

	FORCE_INLINE __mmask16 _mmw_movemask_ps(const __m512 &a)
	{
		__mmask16 mask = _mm512_cmp_epi32_mask(_mm512_and_si512(_mm512_castps_si512(a), _mm512_set1_epi32(0x80000000)), _mm512_set1_epi32(0), 4);	// a & 0x8000000 != 0
		return mask;
	}

	FORCE_INLINE __m512 _mmw_blendv_ps(const __m512 &a, const __m512 &b, const __m512 &c)
	{
		__mmask16 mask = _mmw_movemask_ps(c);
		return _mm512_mask_mov_ps(a, mask, b);
	} 

	static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::AVX512;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Include common algorithm implementation (general, SIMD independent code)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	#include "MaskedOcclusionCullingCommon.inl"

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utility function to create a new object using the allocator callbacks
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	typedef MaskedOcclusionCulling::pfnAlignedAlloc            pfnAlignedAlloc;
	typedef MaskedOcclusionCulling::pfnAlignedFree             pfnAlignedFree;

	MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
	{
		MaskedOcclusionCullingPrivate *object = (MaskedOcclusionCullingPrivate *)alignedAlloc(64, sizeof(MaskedOcclusionCullingPrivate));
		new (object) MaskedOcclusionCullingPrivate(alignedAlloc, alignedFree);
		return object;
	}
};

#else

namespace MaskedOcclusionCullingAVX512
{
	typedef MaskedOcclusionCulling::pfnAlignedAlloc            pfnAlignedAlloc;
	typedef MaskedOcclusionCulling::pfnAlignedFree             pfnAlignedFree;

	MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
	{
		return nullptr;
	}
};

#endif
