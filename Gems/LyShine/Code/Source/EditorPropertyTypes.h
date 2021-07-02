/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
