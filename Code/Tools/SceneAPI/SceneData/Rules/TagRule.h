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
#include <SceneAPI/SceneCore/DataTypes/Rules/ITagRule.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class TagRule
                : public DataTypes::ITagRule
            {
            public:
                AZ_RTTI(TagRule, "{AF678C05-ED7A-4622-9007-A5CC6044C42D}", DataTypes::ITagRule);
                AZ_CLASS_ALLOCATOR_DECL

                ~TagRule() override = default;

                const AZStd::vector<AZStd::string>& GetTags() const override;

                static void Reflect(ReflectContext* context);

            protected:
                AZStd::vector<AZStd::string> m_tags;
            };
        } // SceneData
    } // SceneAPI
} // AZ
