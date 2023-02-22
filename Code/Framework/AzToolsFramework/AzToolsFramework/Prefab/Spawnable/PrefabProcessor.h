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
    class PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabProcessor, AZ::SystemAllocator);
        AZ_RTTI(PrefabProcessor, "{393C95DF-C0DA-4EF0-A081-9CA899649DDD}");

        virtual ~PrefabProcessor() = default;

        virtual void Process(PrefabProcessorContext& context) = 0;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
