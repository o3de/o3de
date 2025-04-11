/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IUnmodifiableRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>

namespace AZ
{
    class ReflectContext;
    namespace SceneAPI
    {
        namespace SceneData
        {
            // Disables UI interaction for the node and children of the node with this rule.
            class SCENE_DATA_CLASS UnmodifiableRule : public DataTypes::IUnmodifiableRule
            {
            public:
                AZ_RTTI(UnmodifiableRule, "{6527EBC2-60DF-4E5A-98B4-106F050A186C}", DataTypes::IUnmodifiableRule);
                AZ_CLASS_ALLOCATOR(UnmodifiableRule, AZ::SystemAllocator)

                SCENE_DATA_API ~UnmodifiableRule() override = default;

                SCENE_DATA_API bool ModifyTooltip(AZStd::string& tooltip) override;

                static void Reflect(ReflectContext* context);
            };
        } // namespace DataTypes
    } // namespace SceneAPI
} // namespace AZ
