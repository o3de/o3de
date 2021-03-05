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
#include <IIndexedMesh.h> // <> required for Interfuscator

STRUCT_INFO_BEGIN(SMeshTexCoord)
STRUCT_VAR_INFO(s, TYPE_INFO(float))
STRUCT_VAR_INFO(t, TYPE_INFO(float))
STRUCT_INFO_END(SMeshTexCoord)

STRUCT_INFO_BEGIN(SMeshNormal)
STRUCT_VAR_INFO(Normal, TYPE_INFO(Vec3))
STRUCT_INFO_END(SMeshNormal)

STRUCT_INFO_BEGIN(SMeshColor)
STRUCT_VAR_INFO(r, TYPE_INFO(uint8))
STRUCT_VAR_INFO(g, TYPE_INFO(uint8))
STRUCT_VAR_INFO(b, TYPE_INFO(uint8))
STRUCT_VAR_INFO(a, TYPE_INFO(uint8))
STRUCT_INFO_END(SMeshColor)

STRUCT_INFO_BEGIN(SMeshTangents)
STRUCT_VAR_INFO(Tangent, TYPE_INFO(Vec4sf))
STRUCT_VAR_INFO(Bitangent, TYPE_INFO(Vec4sf))
STRUCT_INFO_END(SMeshTangents)

STRUCT_INFO_BEGIN(SMeshQTangents)
STRUCT_VAR_INFO(TangentBitangent, TYPE_INFO(Vec4sf))
STRUCT_INFO_END(SMeshQTangents)

STRUCT_INFO_BEGIN(SMeshBoneMapping_uint16)
STRUCT_VAR_INFO(boneIds, TYPE_ARRAY(4, TYPE_INFO(SMeshBoneMapping_uint16::BoneId)))
STRUCT_VAR_INFO(weights, TYPE_ARRAY(4, TYPE_INFO(SMeshBoneMapping_uint16::Weight)))
STRUCT_INFO_END(SMeshBoneMapping_uint16)

STRUCT_INFO_BEGIN(SMeshBoneMapping_uint8)
STRUCT_VAR_INFO(boneIds, TYPE_ARRAY(4, TYPE_INFO(SMeshBoneMapping_uint8::BoneId)))
STRUCT_VAR_INFO(weights, TYPE_ARRAY(4, TYPE_INFO(SMeshBoneMapping_uint8::Weight)))
STRUCT_INFO_END(SMeshBoneMapping_uint8)
