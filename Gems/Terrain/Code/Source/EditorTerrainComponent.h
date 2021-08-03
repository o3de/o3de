/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Module/Module.h>
#include <TerrainComponent.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>

namespace Terrain
{
    class EditorTerrainComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainComponent, TerrainConfig>
        , protected GradientSignal::GradientPreviewContextRequestBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainComponent, TerrainConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainComponent, "{EC7B2DB9-345F-45C6-BA1C-49A58B8112B6}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        AZ::u32 ConfigurationChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // GradientPreviewContextRequestBus::Handler
        AZ::EntityId GetPreviewEntity() const override;
        AZ::Aabb GetPreviewBounds() const override;
        bool GetConstrainToShape() const override;

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain";
        static constexpr const char* const s_componentDescription = "Does something, and that something is hopefully terrain-like.";
        static constexpr const char* const s_icon = "Editor/Icons/Components/SurfaceData.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/SurfaceData.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}
