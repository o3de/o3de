/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorTerrainWorldRendererComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Terrain
{
    void EditorTerrainWorldRendererComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorTerrainWorldRendererComponent, BaseClassType>()
                ->Version(0)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorTerrainWorldRendererComponent>(
                    "Terrain World Renderer", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Terrain")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/TerrainWorldRenderer.svg")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.o3de.org/docs/user-guide/components/reference/terrain/world-renderer/")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/TerrainWorldRenderer.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                ;
            }
        }
    }


    void EditorTerrainWorldRendererComponent::Init()
    {
        BaseClassType::Init();
    }

    void EditorTerrainWorldRendererComponent::Activate()
    {
        BaseClassType::Activate();
    }

    AZ::u32 EditorTerrainWorldRendererComponent::ConfigurationChanged()
    {
        return BaseClassType::ConfigurationChanged();
    }
}
