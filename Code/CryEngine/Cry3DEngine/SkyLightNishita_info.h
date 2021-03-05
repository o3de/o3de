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

#ifndef CRYINCLUDE_CRY3DENGINE_SKYLIGHTNISHITA_INFO_H
#define CRYINCLUDE_CRY3DENGINE_SKYLIGHTNISHITA_INFO_H
#pragma once

#include "SkyLightNishita.h"

STRUCT_INFO_BEGIN(CSkyLightNishita::SOpticalDepthLUTEntry)
STRUCT_VAR_INFO(mie, TYPE_INFO(f32))
STRUCT_VAR_INFO(rayleigh, TYPE_INFO(f32))
STRUCT_INFO_END(CSkyLightNishita::SOpticalDepthLUTEntry)

STRUCT_INFO_BEGIN(CSkyLightNishita::SOpticalScaleLUTEntry)
STRUCT_VAR_INFO(atmosphereLayerHeight, TYPE_INFO(f32))
STRUCT_VAR_INFO(mie, TYPE_INFO(f32))
STRUCT_VAR_INFO(rayleigh, TYPE_INFO(f32))
STRUCT_INFO_END(CSkyLightNishita::SOpticalScaleLUTEntry)


#endif // CRYINCLUDE_CRY3DENGINE_SKYLIGHTNISHITA_INFO_H
