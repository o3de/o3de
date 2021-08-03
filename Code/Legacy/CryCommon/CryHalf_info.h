/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
