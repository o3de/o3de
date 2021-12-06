/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessorContext.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    inline static constexpr AZStd::string_view PlayInEditor = "PlayInEditor";
    inline static constexpr AZStd::string_view TempSpawnables = "TempSpawnables";

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
