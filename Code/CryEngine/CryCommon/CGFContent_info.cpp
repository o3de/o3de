/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "TypeInfo_impl.h"
#include "CGFContent.h"

STRUCT_INFO_BEGIN(TFace)
STRUCT_VAR_INFO(i0, TYPE_INFO(uint16))
STRUCT_VAR_INFO(i1, TYPE_INFO(uint16))
STRUCT_VAR_INFO(i2, TYPE_INFO(uint16))
STRUCT_INFO_END(TFace)

STRUCT_INFO_BEGIN(IntSkinVertex)
STRUCT_VAR_INFO(__obsolete0, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(pos, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(__obsolete2, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(boneIDs, TYPE_ARRAY(4, TYPE_INFO(uint16)))
STRUCT_VAR_INFO(weights, TYPE_ARRAY(4, TYPE_INFO(f32)))
STRUCT_VAR_INFO(color, TYPE_INFO(ColorB))
STRUCT_INFO_END(IntSkinVertex)

STRUCT_INFO_BEGIN(CStoredSkinningInfo)
STRUCT_VAR_INFO(m_nTicksPerFrame, TYPE_INFO(int32))
STRUCT_VAR_INFO(m_secsPerTick, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_nStart, TYPE_INFO(int32))
STRUCT_VAR_INFO(m_nEnd, TYPE_INFO(int32))
STRUCT_VAR_INFO(m_Speed, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_Distance, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_Slope, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_nAssetFlags, TYPE_INFO(int))
STRUCT_VAR_INFO(m_LHeelStart, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_LHeelEnd, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_LToe0Start, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_LToe0End, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_RHeelStart, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_RHeelEnd, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_RToe0Start, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_RToe0End, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_MoveDirection, TYPE_INFO(Vec3))
STRUCT_INFO_END(CStoredSkinningInfo)

STRUCT_INFO_BEGIN(CControllerInfo)
STRUCT_VAR_INFO(m_nControllerID, TYPE_INFO(uint32))
STRUCT_VAR_INFO(m_nPosKeyTimeTrack, TYPE_INFO(uint32))
STRUCT_VAR_INFO(m_nPosTrack, TYPE_INFO(uint32))
STRUCT_VAR_INFO(m_nRotKeyTimeTrack, TYPE_INFO(uint32))
STRUCT_VAR_INFO(m_nRotTrack, TYPE_INFO(uint32))
STRUCT_INFO_END(CControllerInfo)

STRUCT_INFO_BEGIN(UCol)
STRUCT_VAR_INFO(dcolor, TYPE_INFO(uint32))
STRUCT_INFO_END(UCol)

STRUCT_INFO_BEGIN(SVF_P3S_C4B_T2S)
STRUCT_VAR_INFO(xyz, TYPE_INFO(Vec3f16))
STRUCT_VAR_INFO(color, TYPE_INFO(UCol))
STRUCT_VAR_INFO(st, TYPE_INFO(Vec2f16))
STRUCT_INFO_END(SVF_P3S_C4B_T2S)
