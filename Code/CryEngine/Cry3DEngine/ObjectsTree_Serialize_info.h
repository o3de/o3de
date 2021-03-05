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

#ifndef CRYINCLUDE_CRY3DENGINE_OBJECTSTREE_SERIALIZE_INFO_H
#define CRYINCLUDE_CRY3DENGINE_OBJECTSTREE_SERIALIZE_INFO_H
#pragma once

STRUCT_INFO_BEGIN(SRenderNodeChunk)
STRUCT_VAR_INFO(m_WSBBox, TYPE_INFO(AABB))
STRUCT_VAR_INFO(m_nLayerId, TYPE_INFO(uint16))
STRUCT_VAR_INFO(m_cShadowLodBias, TYPE_INFO(int8))
STRUCT_VAR_INFO(m_ucDummy,   TYPE_INFO(uint8))
STRUCT_VAR_INFO(m_dwRndFlags, TYPE_INFO(uint32))
STRUCT_VAR_INFO(m_nObjectTypeIndex, TYPE_INFO(uint16))
STRUCT_VAR_INFO(m_pad16, TYPE_INFO(uint16))
STRUCT_VAR_INFO(m_fViewDistanceMultiplier, TYPE_INFO(float))
STRUCT_VAR_INFO(m_ucLodRatio, TYPE_INFO(uint8))
STRUCT_VAR_INFO(m_pad8, TYPE_INFO(uint8))
STRUCT_VAR_INFO(m_pad16B, TYPE_INFO(uint16))
STRUCT_INFO_END(SRenderNodeChunk)

STRUCT_INFO_BEGIN(SDecalChunk)
STRUCT_BASE_INFO(SRenderNodeChunk)
STRUCT_VAR_INFO(m_projectionType, TYPE_INFO(int16))
STRUCT_VAR_INFO(m_deferred, TYPE_INFO(uint8))
STRUCT_VAR_INFO(m_pad8, TYPE_INFO(uint8))
STRUCT_VAR_INFO(m_depth, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_pos, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(m_normal, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(m_explicitRightUpFront, TYPE_INFO(Matrix33))
STRUCT_VAR_INFO(m_radius, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_nMaterialId, TYPE_INFO(int32))
STRUCT_VAR_INFO(m_nSortPriority, TYPE_INFO(int32))
STRUCT_INFO_END(SDecalChunk)

STRUCT_INFO_BEGIN(SWaterVolumeChunk)
STRUCT_BASE_INFO(SRenderNodeChunk)
STRUCT_VAR_INFO(m_volumeTypeAndMiscBits, TYPE_INFO(int32))
STRUCT_VAR_INFO(m_volumeID, TYPE_INFO(uint64))
STRUCT_VAR_INFO(m_materialID, TYPE_INFO(int32))
STRUCT_VAR_INFO(m_fogDensity, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_fogColor, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(m_fogPlane, TYPE_INFO(Plane))
STRUCT_VAR_INFO(m_fogShadowing, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_caustics, TYPE_INFO(uint8))
STRUCT_VAR_INFO(m_pad8, TYPE_INFO(uint8))
STRUCT_VAR_INFO(m_pad16, TYPE_INFO(uint16))
STRUCT_VAR_INFO(m_causticIntensity, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_causticTiling, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_causticHeight, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_uTexCoordBegin, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_uTexCoordEnd, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_surfUScale, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_surfVScale, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_numVertices, TYPE_INFO(uint32))
STRUCT_VAR_INFO(m_volumeDepth, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_streamSpeed, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_numVerticesPhysAreaContour, TYPE_INFO(uint32))
STRUCT_INFO_END(SWaterVolumeChunk)

STRUCT_INFO_BEGIN(SWaterVolumeVertex)
STRUCT_VAR_INFO(m_xyz, TYPE_INFO(Vec3))
STRUCT_INFO_END(SWaterVolumeVertex)

STRUCT_INFO_BEGIN(SDistanceCloudChunk)
STRUCT_BASE_INFO(SRenderNodeChunk)
STRUCT_VAR_INFO(m_pos, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(m_sizeX, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_sizeY, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_rotationZ, TYPE_INFO(f32))
STRUCT_VAR_INFO(m_materialID, TYPE_INFO(int32))
STRUCT_INFO_END(SDistanceCloudChunk)

#endif // CRYINCLUDE_CRY3DENGINE_OBJECTSTREE_SERIALIZE_INFO_H
