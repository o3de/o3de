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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <EditorDefs.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>
#include "../LegacyTerrainLevelComponent.h"
#include <AzCore/Math/Aabb.h>
#include <Terrain/Bus/TerrainProviderBus.h>


namespace LegacyTerrain
{
    class LegacyTerrainEditorLevelComponent
        : public LmbrCentral::EditorWrappedComponentBase<LegacyTerrainLevelComponent, LegacyTerrainLevelConfig>
        , private Terrain::TerrainSystemServiceRequestBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<LegacyTerrainLevelComponent, LegacyTerrainLevelConfig>;
        AZ_EDITOR_COMPONENT(LegacyTerrainEditorLevelComponent, "{CC1924C2-B708-45C1-8A0C-2B37B2E6A115}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        AZ::u32 ConfigurationChanged() override;


        void RegisterArea(AZ::EntityId areaId) override;
        void UnregisterArea(AZ::EntityId areaId) override;
        void RefreshArea(AZ::EntityId areaId) override;

    protected:
        using BaseClassType::m_configuration;
        using BaseClassType::m_component;
        using BaseClassType::m_visible;

        AZStd::unordered_map<AZ::EntityId, AZ::Aabb> m_registeredAreas;

    private:
    };
}
