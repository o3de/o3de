/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/TerrainWorldRendererComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainWorldRendererComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainWorldRendererComponent, TerrainWorldRendererConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainWorldRendererComponent, TerrainWorldRendererConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainWorldRendererComponent, "{7BEFF763-89A6-4EDA-B199-B049A8E757AF}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        AZ::u32 ConfigurationChanged() override;

    protected:
        using BaseClassType::m_configuration;
        using BaseClassType::m_component;
        using BaseClassType::m_visible;

    private:
    };
}
