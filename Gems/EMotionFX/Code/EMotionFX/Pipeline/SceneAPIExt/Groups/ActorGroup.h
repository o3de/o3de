/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

#include <SceneAPIExt/Groups/IActorGroup.h>

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
            class ActorGroup
                : public IActorGroup
            {
            public:
                AZ_RTTI(ActorGroup, "{D1AC3803-8282-46C5-8610-93CD39B0F843}", IActorGroup);
                AZ_CLASS_ALLOCATOR_DECL

                ActorGroup();
                ~ActorGroup() override = default;

                const AZStd::string& GetName() const override;
                void SetName(const AZStd::string& name);
                void SetName(AZStd::string&& name);
                const AZ::Uuid& GetId() const override;
                void OverrideId(const AZ::Uuid& id);

                AZ::SceneAPI::Containers::RuleContainer& GetRuleContainer() override;
                const AZ::SceneAPI::Containers::RuleContainer& GetRuleContainerConst() const override;

                // IActorGroup overrides
                const AZStd::string& GetSelectedRootBone() const override;
                void SetSelectedRootBone(const AZStd::string& selectedRootBone)  override;
                void SetBestMatchingRootBone(const AZ::SceneAPI::Containers::SceneGraph& sceneGraph) override;

                static void Reflect(AZ::ReflectContext* context);
                static bool IActorGroupVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
                static bool ActorVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            protected:
                AZ::SceneAPI::Containers::RuleContainer             m_rules;
                AZStd::string                                       m_name;
                AZStd::string                                       m_selectedRootBone;
                AZ::Uuid                                            m_id;
            };
        }
    }
}
