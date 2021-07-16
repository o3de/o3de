/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>

#include <SceneAPIExt/Groups/IMotionGroup.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Group
        {
            class MotionGroup
                : public IMotionGroup
            {
            public:
                AZ_RTTI(MotionGroup, "{1B0ABB1E-F6DF-4534-9A35-2DD8244BF58C}", IMotionGroup);
                AZ_CLASS_ALLOCATOR_DECL
                
                MotionGroup();
                ~MotionGroup() override = default;
                void SetName(const AZStd::string& name);
                void SetName(AZStd::string&& name);
                const AZ::Uuid& GetId() const override;
                void OverrideId(const AZ::Uuid& id);

                AZ::SceneAPI::Containers::RuleContainer& GetRuleContainer() override;
                const AZ::SceneAPI::Containers::RuleContainer& GetRuleContainerConst() const override;

                // IMotionGroup overrides
                const AZStd::string& GetSelectedRootBone() const override;
                void SetSelectedRootBone(const AZStd::string& selectedRootBone)  override;

                // IGroup overrides
                const AZStd::string& GetName() const override;

                static void Reflect(AZ::ReflectContext* context);
                static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            protected:
                AZ::SceneAPI::Containers::RuleContainer   m_rules;
                AZStd::string                             m_name;
                AZStd::string                             m_selectedRootBone;
                AZ::Uuid                                  m_id;
            };
        }
    }
}
