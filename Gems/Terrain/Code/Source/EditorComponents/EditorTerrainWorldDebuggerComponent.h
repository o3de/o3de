/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/TerrainWorldDebuggerComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainWorldDebuggerComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainWorldDebuggerComponent, TerrainWorldDebuggerConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainWorldDebuggerComponent, TerrainWorldDebuggerConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainWorldDebuggerComponent, "{D09BA0B9-FB51-446B-BD7B-3C40743D2E39}", BaseClassType);
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
