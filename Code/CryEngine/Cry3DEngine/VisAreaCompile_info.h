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

#ifndef CRYINCLUDE_CRY3DENGINE_VISAREACOMPILE_INFO_H
#define CRYINCLUDE_CRY3DENGINE_VISAREACOMPILE_INFO_H
#pragma once

STRUCT_INFO_BEGIN(SVisAreaChunk)
STRUCT_VAR_INFO(nChunkVersion, TYPE_INFO(int))
STRUCT_VAR_INFO(boxArea, TYPE_INFO(AABB))
STRUCT_VAR_INFO(boxStatics, TYPE_INFO(AABB))
STRUCT_VAR_INFO(sName, TYPE_ARRAY(32, TYPE_INFO(char)))
STRUCT_VAR_INFO(nObjectsBlockSize, TYPE_INFO(int))
STRUCT_VAR_INFO(arrConnectionsId, TYPE_ARRAY(30, TYPE_INFO(int)))
STRUCT_VAR_INFO(dwFlags, TYPE_INFO(uint32))
STRUCT_VAR_INFO(fPortalBlending, TYPE_INFO(float))
STRUCT_VAR_INFO(vConnNormals, TYPE_ARRAY(2, TYPE_INFO(Vec3)))
STRUCT_VAR_INFO(fHeight, TYPE_INFO(float))
STRUCT_VAR_INFO(vAmbColor, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(fViewDistRatio, TYPE_INFO(float))
STRUCT_INFO_END(SVisAreaChunk)


#endif // CRYINCLUDE_CRY3DENGINE_VISAREACOMPILE_INFO_H
