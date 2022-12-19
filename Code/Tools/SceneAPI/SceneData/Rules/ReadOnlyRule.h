#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IReadOnlyRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>

namespace AZ
{
    class ReflectContext;
    namespace SceneAPI
    {
        namespace SceneData
        {
            // Disables UI interaction for the node and children of the node with this rule.
            class SCENE_DATA_CLASS ReadOnlyRule : public DataTypes::IReadOnlyRule
            {
            public:
                AZ_RTTI(ReadOnlyRule, "{6527EBC2-60DF-4E5A-98B4-106F050A186C}", DataTypes::IReadOnlyRule);
                AZ_CLASS_ALLOCATOR(ReadOnlyRule, AZ::SystemAllocator, 0)

                SCENE_DATA_API ~ReadOnlyRule() override = default;

                SCENE_DATA_API bool ModifyTooltip(AZStd::string& tooltip) override;

                static void Reflect(ReflectContext* context);
            };
        } // namespace DataTypes
    } // namespace SceneAPI
} // namespace AZ
