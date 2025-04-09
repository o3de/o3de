/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/Groups/ImportGroup.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::SceneAPI::SceneData
{
    ImportGroup::ImportGroup()
        : m_name("Import Settings")
        , m_id(Uuid::CreateRandom())
    {
    }

    const AZStd::string& ImportGroup::GetName() const
    {
        return m_name;
    }

    const Uuid& ImportGroup::GetId() const
    {
        return m_id;
    }

    Containers::RuleContainer& ImportGroup::GetRuleContainer()
    {
        return m_rules;
    }

    const Containers::RuleContainer& ImportGroup::GetRuleContainerConst() const
    {
        return m_rules;
    }

    DataTypes::ISceneNodeSelectionList& ImportGroup::GetSceneNodeSelectionList()
    {
        return m_nodeSelectionList;
    }

    const DataTypes::ISceneNodeSelectionList& ImportGroup::GetSceneNodeSelectionList() const
    {
        return m_nodeSelectionList;
    }

    const SceneImportSettings& ImportGroup::GetImportSettings() const
    {
        return m_importSettings;
    }

    void ImportGroup::SetImportSettings(const SceneImportSettings& importSettings)
    {
        m_importSettings = importSettings;
    }

    void ImportGroup::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context); serializeContext)
        {
            serializeContext->Class<ImportGroup, DataTypes::IImportGroup>()
                ->Version(0)
                ->Field("ImportSettings", &ImportGroup::m_importSettings)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext)
            {
                editContext->Class<ImportGroup>("Import Settings",
                        "The import group controls the Asset Importer settings. "
                        "These settings affect how the source data is processed before being handled by the scene exporters.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::Max, 1)
                        ->Attribute(AZ::Edit::Attributes::CategoryStyle, "display divider")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/assets/scene-settings/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImportGroup::m_importSettings,
                        "Import Settings",
                        "Settings that affect how the scene data is transformed when it is read in.")
                ;
            }
        }
    }
}
