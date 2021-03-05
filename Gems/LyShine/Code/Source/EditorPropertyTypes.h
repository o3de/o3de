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
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Component/Entity.h>

namespace LyShine
{
    using AZu32ComboBoxVec = AZStd::vector<AZStd::pair<AZ::u32, AZStd::string> >;

    //! Returns a string enumeration list for the given min/max value ranges
    AZu32ComboBoxVec GetEnumSpriteIndexList(AZ::EntityId entityId, AZ::u32 indexMin, AZ::u32 indexMax, const char* errorMessage = "");
}