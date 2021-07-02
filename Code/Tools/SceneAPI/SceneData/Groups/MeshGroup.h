/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>


namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace SceneData
        {
            class MeshGroup
                : public DataTypes::IMeshGroup
            {
            public:
                AZ_RTTI(MeshGroup, "{07B356B7-3635-40B5-878A-FAC4EFD5AD86}", DataTypes::IMeshGroup);
                AZ_CLASS_ALLOCATOR(MeshGroup, SystemAllocator, 0)

                MeshGroup();
                ~MeshGroup() override = default;

                const AZStd::string& GetName() const override;
                void SetName(const AZStd::string& name);
                void SetName(AZStd::string&& name) override;
                const Uuid& GetId() const override;
                void OverrideId(const Uuid& id) override;

                Containers::RuleContainer& GetRuleContainer() override;
                const Containers::RuleContainer& GetRuleContainerConst() const override;

                DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
                const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

                static void Reflect(AZ::ReflectContext* context);
                static bool VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement);

            protected:
                SceneNodeSelectionList m_nodeSelectionList;
                Containers::RuleContainer m_rules;
                AZStd::string m_name;
                Uuid m_id;
            };
        } // SceneData
    } // SceneAPI
} // AZ
