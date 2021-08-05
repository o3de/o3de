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

#if defined(__MICROSOFT_COMPILER) && _MSC_VER < 1900
	// If you remove/comment this error, the code will compile & use the SSE41 version instead. 
	#error Older versions than visual studio 2015 not supported due to compiler bug(s)
#endif

#if !defined(__MICROSOFT_COMPILER) || _MSC_VER >= 1900

// For performance reasons, the MaskedOcclusionCullingAVX2.cpp file should be compiled with VEX encoding for SSE instructions (to avoid 
// AVX-SSE transition penalties, see https://software.intel.com/en-us/articles/avoiding-avx-sse-transition-penalties). However, the SSE
// version in MaskedOcclusionCulling.cpp _must_ be compiled without VEX encoding to allow backwards compatibility. Best practice is to 
// use lowest supported target platform (e.g. /arch:SSE2) as project default, and elevate only the MaskedOcclusionCullingAVX2/512.cpp files.
#ifndef __AVX2__
	#error For best performance, MaskedOcclusionCullingAVX2.cpp should be compiled with /arch:AVX2
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AVX specific defines and constants
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIMD_LANES             8
#define TILE_HEIGHT_SHIFT      3

#define SIMD_LANE_IDX _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7)

#define SIMD_SUB_TILE_COL_OFFSET _mm256_setr_epi32(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET _mm256_setr_epi32(0, 0, 0, 0, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT)
#define SIMD_SUB_TILE_COL_OFFSET_F _mm256_setr_ps(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET_F _mm256_setr_ps(0, 0, 0, 0, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT)

#define SIMD_SHUFFLE_SCANLINE_TO_SUBTILES _mm256_setr_epi8(0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF, 0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF)

#define SIMD_LANE_YCOORD_I _mm256_setr_epi32(128, 384, 640, 896, 1152, 1408, 1664, 1920)
#define SIMD_LANE_YCOORD_F _mm256_setr_ps(128.0f, 384.0f, 640.0f, 896.0f, 1152.0f, 1408.0f, 1664.0f, 1920.0f)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AVX specific typedefs and functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef __m256 __mw;
typedef __m256i __mwi;

#define _mmw_set1_ps                _mm256_set1_ps
#define _mmw_setzero_ps             _mm256_setzero_ps
#define _mmw_and_ps                 _mm256_and_ps
#define _mmw_or_ps                  _mm256_or_ps
#define _mmw_xor_ps                 _mm256_xor_ps
#define _mmw_not_ps(a)              _mm256_xor_ps((a), _mm256_castsi256_ps(_mm256_set1_epi32(~0)))
#define _mmw_andnot_ps              _mm256_andnot_ps
#define _mmw_neg_ps(a)              _mm256_xor_ps((a), _mm256_set1_ps(-0.0f))
#define _mmw_abs_ps(a)              _mm256_and_ps((a), _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF)))
#define _mmw_add_ps                 _mm256_add_ps
#define _mmw_sub_ps                 _mm256_sub_ps
#define _mmw_mul_ps                 _mm256_mul_ps
#define _mmw_div_ps                 _mm256_div_ps
#define _mmw_min_ps                 _mm256_min_ps
#define _mmw_max_ps                 _mm256_max_ps
#define _mmw_fmadd_ps               _mm256_fmadd_ps
#define _mmw_fmsub_ps               _mm256_fmsub_ps
#define _mmw_movemask_ps            _mm256_movemask_ps
#define _mmw_blendv_ps              _mm256_blendv_ps
#define _mmw_cmpge_ps(a,b)          _mm256_cmp_ps(a, b, _CMP_GE_OQ)
#define _mmw_cmpgt_ps(a,b)          _mm256_cmp_ps(a, b, _CMP_GT_OQ)
#define _mmw_cmpeq_ps(a,b)          _mm256_cmp_ps(a, b, _CMP_EQ_OQ)
#define _mmw_floor_ps(x)            _mm256_round_ps(x, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC)
#define _mmw_ceil_ps(x)             _mm256_round_ps(x, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC)
#define _mmw_shuffle_ps             _mm256_shuffle_ps
#define _mmw_insertf32x4_ps         _mm256_insertf128_ps
#define _mmw_cvtepi32_ps            _mm256_cvtepi32_ps
#define _mmw_blendv_epi32(a,b,c)    simd_cast<__mwi>(_mmw_blendv_ps(simd_cast<__mw>(a), simd_cast<__mw>(b), simd_cast<__mw>(c)))

#define _mmw_set1_epi32             _mm256_set1_epi32
#define _mmw_setzero_epi32          _mm256_setzero_si256
#define _mmw_and_epi32              _mm256_and_si256
#define _mmw_or_epi32               _mm256_or_si256
#define _mmw_xor_epi32              _mm256_xor_si256
#define _mmw_not_epi32(a)           _mm256_xor_si256((a), _mm256_set1_epi32(~0))
#define _mmw_andnot_epi32           _mm256_andnot_si256
#define _mmw_neg_epi32(a)           _mm256_sub_epi32(_mm256_set1_epi32(0), (a))
#define _mmw_add_epi32              _mm256_add_epi32
#define _mmw_sub_epi32              _mm256_sub_epi32
#define _mmw_min_epi32              _mm256_min_epi32
#define _mmw_max_epi32              _mm256_max_epi32
#define _mmw_subs_epu16             _mm256_subs_epu16
#define _mmw_mullo_epi32            _mm256_mullo_epi32
#define _mmw_cmpeq_epi32            _mm256_cmpeq_epi32
#define _mmw_testz_epi32            _mm256_testz_si256
#define _mmw_cmpgt_epi32            _mm256_cmpgt_epi32
#define _mmw_srai_epi32             _mm256_srai_epi32
#define _mmw_srli_epi32             _mm256_srli_epi32
#define _mmw_slli_epi32             _mm256_slli_epi32
#define _mmw_sllv_ones(x)           _mm256_sllv_epi32(SIMD_BITS_ONE, x)
#define _mmw_transpose_epi8(x)      _mm256_shuffle_epi8(x, SIMD_SHUFFLE_SCANLINE_TO_SUBTILES)
#define _mmw_abs_epi32              _mm256_abs_epi32
#define _mmw_cvtps_epi32            _mm256_cvtps_epi32
#define _mmw_cvttps_epi32           _mm256_cvttps_epi32

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specialized AVX input assembly function for general vertex gather 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef MaskedOcclusionCulling::VertexLayout VertexLayout;

FORCE_INLINE void GatherVertices(__m256 *vtxX, __m256 *vtxY, __m256 *vtxW, const float *inVtx, const unsigned int *inTrisPtr, int numLanes, const VertexLayout &vtxLayout)
{
	assert(numLanes >= 1);

	const __m256i SIMD_TRI_IDX_OFFSET = _mm256_setr_epi32(0, 3, 6, 9, 12, 15, 18, 21);
	static const __m256i SIMD_LANE_MASK[9] = {
		_mm256_setr_epi32( 0,  0,  0,  0,  0,  0,  0,  0),
		_mm256_setr_epi32(~0,  0,  0,  0,  0,  0,  0,  0),
		_mm256_setr_epi32(~0, ~0,  0,  0,  0,  0,  0,  0),
		_mm256_setr_epi32(~0, ~0, ~0,  0,  0,  0,  0,  0),
		_mm256_setr_epi32(~0, ~0, ~0, ~0,  0,  0,  0,  0),
		_mm256_setr_epi32(~0, ~0, ~0, ~0, ~0,  0,  0,  0),
		_mm256_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0,  0,  0),
		_mm256_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0,  0),
		_mm256_setr_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0)
	};

	// Compute per-lane index list offset that guards against out of bounds memory accesses
	__m256i safeTriIdxOffset = _mm256_and_si256(SIMD_TRI_IDX_OFFSET, SIMD_LANE_MASK[numLanes]);

	// Fetch triangle indices. 
	__m256i vtxIdx[3];
	vtxIdx[0] = _mmw_mullo_epi32(_mm256_i32gather_epi32((const int*)inTrisPtr + 0, safeTriIdxOffset, 4), _mmw_set1_epi32(vtxLayout.mStride));
	vtxIdx[1] = _mmw_mullo_epi32(_mm256_i32gather_epi32((const int*)inTrisPtr + 1, safeTriIdxOffset, 4), _mmw_set1_epi32(vtxLayout.mStride));
	vtxIdx[2] = _mmw_mullo_epi32(_mm256_i32gather_epi32((const int*)inTrisPtr + 2, safeTriIdxOffset, 4), _mmw_set1_epi32(vtxLayout.mStride));

	char *vPtr = (char *)inVtx;

	// Fetch triangle vertices
	for (int i = 0; i < 3; i++)
	{
		vtxX[i] = _mm256_i32gather_ps((float *)vPtr, vtxIdx[i], 1);
		vtxY[i] = _mm256_i32gather_ps((float *)(vPtr + vtxLayout.mOffsetY), vtxIdx[i], 1);
		vtxW[i] = _mm256_i32gather_ps((float *)(vPtr + vtxLayout.mOffsetW), vtxIdx[i], 1);
	}
}

namespace MaskedOcclusionCullingAVX2
{
	static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::AVX2;

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

namespace MaskedOcclusionCullingAVX2
{
	typedef MaskedOcclusionCulling::pfnAlignedAlloc            pfnAlignedAlloc;
	typedef MaskedOcclusionCulling::pfnAlignedFree             pfnAlignedFree;

	MaskedOcclusionCulling *CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
	{
		return nullptr;
	}
};

#endif
