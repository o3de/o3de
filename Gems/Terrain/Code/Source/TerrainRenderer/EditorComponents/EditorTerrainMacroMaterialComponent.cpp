/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/EditorComponents/EditorTerrainMacroMaterialComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

namespace Terrain
{
    void EditorTerrainMacroMaterialComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::ReflectSubClass<EditorTerrainMacroMaterialComponent, BaseClassType>(
            context, 1,
            &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType,
            typename BaseClassType::WrappedConfigType, 1>
        );

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            AZ::EditContext* editContext = serializeContext->GetEditContext();

            // The edit context for TerrainMacroMaterialConfig is specified here to make it easier to add custom filtering to the
            // asset picker for the material asset so that we can eventually only display materials that inherit from the proper
            // material type.
            if (editContext)
            {
                editContext
                    ->Class<TerrainMacroMaterialConfig>(
                        "Terrain Macro Material Component", "Provide a terrain macro material for a region of the world")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_materialAsset, "Macro Material",
                        "Terrain macro material for use by any terrain inside the bounding box on this entity.")
                        // This is disabled until ChangeValidate can support the Asset<T> type. :(
                        //->Attribute(AZ::Edit::Attributes::ChangeValidate, &TerrainMacroMaterialConfig::ValidateMaterialAsset)
                    ;
            }
        }

    }
}
