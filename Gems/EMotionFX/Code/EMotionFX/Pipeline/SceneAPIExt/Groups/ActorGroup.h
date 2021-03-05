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

                // ISceneNodeGroup overrides
                // This list should contain all the nodes that should be exported in this actor. ( base nodes + all LOD nodes )
                // To get the base node list, use the function in below.
                AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
                const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

                // This list should contain the base nodes (LOD0 nodes)
                AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetBaseNodeSelectionList() override;
                const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetBaseNodeSelectionList() const override;

                // IActorGroup overrides
                const AZStd::string& GetSelectedRootBone() const override;

                void SetSelectedRootBone(const AZStd::string& selectedRootBone)  override;

                static void Reflect(AZ::ReflectContext* context);
                static bool SceneNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
                static bool ActorVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            protected:
                AZ::SceneAPI::SceneData::SceneNodeSelectionList     m_allNodeSelectionList;
                AZ::SceneAPI::SceneData::SceneNodeSelectionList     m_baseNodeSelectionList;
                AZ::SceneAPI::Containers::RuleContainer             m_rules;
                AZStd::string                                       m_name;
                AZStd::string                                       m_selectedRootBone;
                AZ::Uuid                                            m_id;
            };
        }
    }
}