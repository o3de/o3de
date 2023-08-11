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
        AZ_Info("ImportGroup", "Constructing ImportGroup.");
    }

    ImportGroup::~ImportGroup()
    {
        AZ_Info("ImportGroup", "Destroying ImportGroup.");
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

    void ImportGroup::Reflect(ReflectContext* context)
    {
        
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context); serializeContext)
        {
            serializeContext->Class<ImportGroup, DataTypes::IImportGroup>()
                ->Version(0)
                ->Field("OptimizeScene", &ImportGroup::m_optimizeScene)
                ->Field("OptimizeMeshes", &ImportGroup::m_optimizeMeshes)
                ;

            static constexpr const char* importTooltip = "The import group controls the Asset Importer settings. "
                "These settings affect how the data is read into memory and processed before being exported by the various scene exporters. "
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext)
            {
                editContext->Class<ImportGroup>("ImportSettings", importTooltip)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::Max, 1)
                        ->Attribute(AZ::Edit::Attributes::CategoryStyle, "display divider")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/assets/scene-settings/")
                    ->UIElement(AZ::Edit::UIHandlers::MultiLineEdit, "", "")
                        ->Attribute(AZ::Edit::Attributes::ValueText,
                            "The Import Settings affect how the scene data is transformed when importing the data into memory, "
                            "before the data is exported through the various scene exporters (Meshes, Actors, PhysX Meshes, etc). "
                        )
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImportGroup::m_optimizeScene,
                        "Optimize Scene",
                        "Nodes without animations, bones, lights, or cameras assigned are collapsed and joined. "
                        "This is useful for non-optimized files that have hundreds or thousands of nodes within them that aren't "
                        "needed to remain separate in O3DE. This should not be used on files where the nodes need to remain separate "
                        "for individual submesh control and transformations."
                        )
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImportGroup::m_optimizeMeshes,
                        "Optimize Meshes",
                        "This attempts to reduce the total number of meshes in the scene.")
                ;
            }
        }
    }
}
