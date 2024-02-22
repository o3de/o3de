/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(CARBONATED)

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace LmbrCentral
{
    /**
     * Editor spawner component
     * Spawns the entities from a ".spawnable" (prefab) asset at runtime.
     */
    class EditorPrefabSpawnerComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorPrefabSpawnerComponent, "{55BA3411-A45A-DE41-C456-ABC4567C8AB1}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType&);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType&);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType&);

        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:

        //////////////////////////////////////////////////////////////////////////
        // Data changed validation methods
        AZ::u32 PrefabAssetChanged();
        AZ::u32 SpawnOnActivateChanged();
        bool HasInfiniteLoop();

        //////////////////////////////////////////////////////////////////////////
        // Serialized members
        AZ::Data::Asset<AzFramework::Spawnable> m_prefabAsset { AZ::Data::AssetLoadBehavior::PreLoad };
        bool m_spawnOnActivate = false;
        bool m_destroyOnDeactivate = false;
    };

} // namespace LmbrCentral

#endif
