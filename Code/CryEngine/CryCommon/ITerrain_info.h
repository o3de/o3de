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

#ifndef CRYINCLUDE_CRY3DENGINE_ITERRAIN_INFO_H
#define CRYINCLUDE_CRY3DENGINE_ITERRAIN_INFO_H
#pragma once

#include <TypeInfo_impl.h>
#include <ITerrain.h>

STRUCT_INFO_BEGIN(ITerrain::SurfaceWeight)
STRUCT_VAR_INFO(Ids, TYPE_ARRAY(WeightCount, TYPE_INFO(uint8)))
STRUCT_VAR_INFO(Weights, TYPE_ARRAY(WeightCount, TYPE_INFO(uint8)))
STRUCT_INFO_END(ITerrain::SurfaceWeight)

#endif
