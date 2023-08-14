/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SceneAPI/SceneCore/DataTypes/Groups/IImportGroup.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::SceneAPI::SceneData
{
    class ImportGroup final
        : public DataTypes::IImportGroup
    {
    public:
        AZ_RTTI(ImportGroup, "{41DCBEAB-203C-4A05-96FA-98E1D8A96FA1}", DataTypes::IImportGroup);
        AZ_CLASS_ALLOCATOR(ImportGroup, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        ImportGroup();
        ~ImportGroup() override = default;

        const AZStd::string& GetName() const override;
        const Uuid& GetId() const override;

        Containers::RuleContainer& GetRuleContainer() override;
        const Containers::RuleContainer& GetRuleContainerConst() const override;

        DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
        const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

        const SceneImportSettings& GetImportSettings() const override;

    private:
        SceneImportSettings m_importSettings;

        SceneNodeSelectionList m_nodeSelectionList;
        Containers::RuleContainer m_rules;
        AZStd::string m_name;
        Uuid m_id;
    };
}
