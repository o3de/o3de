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

#ifndef CRYINCLUDE_CRYCOMMON_CRYHALF_INFO_H
#define CRYINCLUDE_CRYCOMMON_CRYHALF_INFO_H
#pragma once

#include "CryHalf.inl"

STRUCT_INFO_BEGIN(CryHalf2)
STRUCT_VAR_INFO(x, TYPE_INFO(CryHalf))
STRUCT_VAR_INFO(y, TYPE_INFO(CryHalf))
STRUCT_INFO_END(CryHalf2)

STRUCT_INFO_BEGIN(CryHalf4)
STRUCT_VAR_INFO(x, TYPE_INFO(CryHalf))
STRUCT_VAR_INFO(y, TYPE_INFO(CryHalf))
STRUCT_VAR_INFO(z, TYPE_INFO(CryHalf))
STRUCT_VAR_INFO(w, TYPE_INFO(CryHalf))
STRUCT_INFO_END(CryHalf4)

#endif // CRYINCLUDE_CRYCOMMON_CRYHALF_INFO_H
