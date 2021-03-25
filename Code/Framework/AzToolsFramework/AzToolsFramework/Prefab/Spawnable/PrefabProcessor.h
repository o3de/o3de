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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessorContext.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabProcessor, AZ::SystemAllocator, 0);
        AZ_RTTI(PrefabProcessor, "{393C95DF-C0DA-4EF0-A081-9CA899649DDD}");

        virtual ~PrefabProcessor() = default;

        virtual void Process(PrefabProcessorContext& context) = 0;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
