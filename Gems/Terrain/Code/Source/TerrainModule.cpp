/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <TerrainModule.h>
#include <Components/TerrainSystemComponent.h>
#include <Components/TerrainWorldComponent.h>
#include <Components/TerrainWorldDebuggerComponent.h>
#include <Components/TerrainWorldRendererComponent.h>
#include <Components/TerrainHeightGradientListComponent.h>
#include <Components/TerrainLayerSpawnerComponent.h>
#include <Components/TerrainSurfaceGradientListComponent.h>
#include <Components/TerrainSurfaceDataSystemComponent.h>
#include <Components/TerrainPhysicsColliderComponent.h>
#include <TerrainRenderer/Components/TerrainSurfaceMaterialsListComponent.h>
#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>

namespace Terrain
{
    TerrainModule::TerrainModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
                TerrainSurfaceMaterialsListComponent::CreateDescriptor(),
                TerrainSystemComponent::CreateDescriptor(),
                TerrainWorldComponent::CreateDescriptor(),
                TerrainWorldDebuggerComponent::CreateDescriptor(),
                TerrainWorldRendererComponent::CreateDescriptor(),
                TerrainHeightGradientListComponent::CreateDescriptor(),
                TerrainLayerSpawnerComponent::CreateDescriptor(),
                TerrainMacroMaterialComponent::CreateDescriptor(),
                TerrainSurfaceGradientListComponent::CreateDescriptor(),
                TerrainSurfaceDataSystemComponent::CreateDescriptor(),
                TerrainPhysicsColliderComponent::CreateDescriptor()
            });
    }

    AZ::ComponentTypeList TerrainModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<TerrainSystemComponent>(),
            azrtti_typeid<TerrainSurfaceDataSystemComponent>(),
        };
    }
}

#if !defined(TERRAIN_EDITOR)
#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Terrain::TerrainModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Terrain, Terrain::TerrainModule)
#endif
#endif

