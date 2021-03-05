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

#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_H

#include <Cry_Math.h>
#include <Cry_Color.h>

namespace Serialization {
    class IArchive;
}

struct SkeletonAlias;
bool Serialize(Serialization::IArchive& ar, SkeletonAlias& value, const char* name, const char* label);

#include <Serialization/STL.h>
#include <Serialization/Math.h>
#include <Serialization/Color.h>
#include <Serialization/Decorators/BitFlags.h>
using Serialization::BitFlags;
#include <Serialization/Decorators/Slider.h>
#include <Serialization/Decorators/Range.h>
#include "Serialization/Decorators/ToggleButton.h"
#include "Serialization/Qt.h"
#include <Serialization/ClassFactory.h>
#include <Serialization/Enum.h>

#include <Serialization/IArchive.h>

using Serialization::IArchive;

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_H
