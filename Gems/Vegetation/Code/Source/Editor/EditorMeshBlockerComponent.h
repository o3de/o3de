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

#include <AzCore/Module/Module.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <Components/MeshBlockerComponent.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Vegetation/Editor/EditorAreaComponentBase.h>

namespace Vegetation
{
    class EditorMeshBlockerComponent
        : public EditorAreaComponentBase<MeshBlockerComponent, MeshBlockerConfig>
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        using DerivedClassType = EditorMeshBlockerComponent;
        using BaseClassType = EditorAreaComponentBase<MeshBlockerComponent, MeshBlockerConfig>;
        AZ_EDITOR_COMPONENT(EditorMeshBlockerComponent, EditorMeshBlockerComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Layer Blocker (Mesh)";
        static constexpr const char* const s_componentDescription = "Prevents vegetation from being placed in the mesh";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-layer-blocker-mesh";

    private:
        bool m_drawDebugBounds = false;
    };
}