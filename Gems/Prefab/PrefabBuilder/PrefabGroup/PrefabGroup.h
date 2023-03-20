/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PrefabGroup/IPrefabGroup.h>
#include <AzToolsFramework/Prefab/Procedural/ProceduralPrefabAsset.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::SceneAPI::Containers
{
    class Scene;
}

namespace AZ::SceneAPI::SceneData
{
    class PrefabGroup final
        : public DataTypes::IPrefabGroup
    {
    public:
        AZ_RTTI(PrefabGroup, "{99FE3C6F-5B55-4D8B-8013-2708010EC715}", DataTypes::IPrefabGroup);
        AZ_CLASS_ALLOCATOR(PrefabGroup, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        PrefabGroup();
        ~PrefabGroup() override = default;

        // DataTypes::IPrefabGroup
        AzToolsFramework::Prefab::PrefabDomConstReference GetPrefabDomRef() const override;
        const AZStd::string& GetName() const override;
        const Uuid& GetId() const override;
        Containers::RuleContainer& GetRuleContainer() override;
        const Containers::RuleContainer& GetRuleContainerConst() const override;
        DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
        const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

        // IManifestObject
        void GetManifestObjectsToRemoveOnRemoved(
            AZStd::vector<const IManifestObject*>& toRemove, const AZ::SceneAPI::Containers::SceneManifest& manifest) const override;

        // Concrete API
        void SetId(Uuid id);
        void SetName(AZStd::string name);
        void SetPrefabDom(AzToolsFramework::Prefab::PrefabDom prefabDom);

    private:
        SceneNodeSelectionList m_nodeSelectionList;
        Containers::RuleContainer m_rules;
        AZStd::string m_name;
        Uuid m_id;
        AZStd::shared_ptr<Prefab::PrefabDomData> m_prefabDomData;
    };

    //! If this IRule ends up in a MeshGroup container's rule group,
    //! then the MeshGroup was created by the procedural prefab group logic.
    class ProceduralMeshGroupRule final
        : public AZ::SceneAPI::DataTypes::IRule
    {
    public:
        AZ_RTTI(ProceduralMeshGroupRule, "{8A224146-FBA5-414F-AA98-DA57F86738CD}", IRule);
        AZ_CLASS_ALLOCATOR(ProceduralMeshGroupRule, AZ::SystemAllocator)

        ProceduralMeshGroupRule() = default;
        ~ProceduralMeshGroupRule() override = default;

        bool ModifyTooltip(AZStd::string& tooltip) override;
    };
}
