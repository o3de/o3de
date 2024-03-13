/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorTerrainWorldDebuggerComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Terrain
{
    void EditorTerrainWorldDebuggerComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorTerrainWorldDebuggerComponent, BaseClassType>()
                ->Version(0)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorTerrainWorldDebuggerComponent>(
                    "Terrain World Debugger", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Terrain")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/TerrainWorldDebugger.svg")
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.o3de.org/docs/user-guide/components/reference/terrain/world-debugger/")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/TerrainWorldDebugger.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                ;
            }
        }
    }


    void EditorTerrainWorldDebuggerComponent::Init()
    {
        BaseClassType::Init();
    }

    void EditorTerrainWorldDebuggerComponent::Activate()
    {
        BaseClassType::Activate();
    }

    AZ::u32 EditorTerrainWorldDebuggerComponent::ConfigurationChanged()
    {
        return BaseClassType::ConfigurationChanged();
    }
}
