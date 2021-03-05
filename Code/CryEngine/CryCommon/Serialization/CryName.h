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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYNAME_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYNAME_H
#pragma once

namespace Serialization {
    class IArchive;
}

inline bool Serialize(Serialization::IArchive & ar, class CCryName & cryName, const char* name, const char* label);

#include "CryNameImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_CRYNAME_H
