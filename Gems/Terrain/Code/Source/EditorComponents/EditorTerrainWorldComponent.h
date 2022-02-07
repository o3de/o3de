/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/TerrainWorldComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainWorldComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainWorldComponent, TerrainWorldConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainWorldComponent, TerrainWorldConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainWorldComponent, "{43D02ADC-111F-4584-B590-FF6DC9FC912C}", BaseClassType);
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
