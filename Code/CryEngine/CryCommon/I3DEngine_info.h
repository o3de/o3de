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

#ifndef CRYINCLUDE_CRYCOMMON_I3DENGINE_INFO_H
#define CRYINCLUDE_CRYCOMMON_I3DENGINE_INFO_H
#pragma once

#include "TypeInfo_impl.h"
#include "IShader_info.h"

STRUCT_INFO_BEGIN(SVisAreaManChunkHeader)
STRUCT_VAR_INFO(nVersion, TYPE_INFO(int8))
STRUCT_VAR_INFO(nDummy, TYPE_INFO(int8))
STRUCT_VAR_INFO(nFlags, TYPE_INFO(int8))
STRUCT_VAR_INFO(nFlags2, TYPE_INFO(int8))
STRUCT_VAR_INFO(nChunkSize, TYPE_INFO(int))
STRUCT_VAR_INFO(nVisAreasNum, TYPE_INFO(int))
STRUCT_VAR_INFO(nPortalsNum, TYPE_INFO(int))
STRUCT_VAR_INFO(nOcclAreasNum, TYPE_INFO(int))
STRUCT_INFO_END(SVisAreaManChunkHeader)

STRUCT_INFO_BEGIN(SOcTreeNodeChunk)
STRUCT_VAR_INFO(nChunkVersion, TYPE_INFO(int16))
STRUCT_VAR_INFO(ucChildsMask, TYPE_INFO(int16))
STRUCT_VAR_INFO(nodeBox, TYPE_INFO(AABB))
STRUCT_VAR_INFO(nObjectsBlockSize, TYPE_INFO(int32))
STRUCT_INFO_END(SOcTreeNodeChunk)

STRUCT_INFO_BEGIN(SHotUpdateInfo)
STRUCT_VAR_INFO(nHeigtmap, TYPE_INFO(uint32))
STRUCT_VAR_INFO(nObjTypeMask, TYPE_INFO(uint32))
STRUCT_VAR_INFO(areaBox, TYPE_INFO(AABB))
STRUCT_INFO_END(SHotUpdateInfo)

STRUCT_INFO_BEGIN(SCommonFileHeader)
STRUCT_VAR_INFO(signature, TYPE_ARRAY(4, TYPE_INFO(char)))
STRUCT_VAR_INFO(file_type, TYPE_INFO(uint8))
STRUCT_VAR_INFO(flags, TYPE_INFO(uint8))
STRUCT_VAR_INFO(version, TYPE_INFO(uint16))
STRUCT_INFO_END(SCommonFileHeader)

#endif // CRYINCLUDE_CRYCOMMON_I3DENGINE_INFO_H
