/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorTerrainModule.h>
#include <EditorComponents/EditorTerrainHeightGradientListComponent.h>
#include <EditorComponents/EditorTerrainLayerSpawnerComponent.h>
#include <EditorComponents/EditorTerrainSurfaceGradientListComponent.h>
#include <EditorComponents/EditorTerrainSystemComponent.h>
#include <EditorComponents/EditorTerrainWorldComponent.h>
#include <EditorComponents/EditorTerrainWorldDebuggerComponent.h>
#include <EditorComponents/EditorTerrainPhysicsColliderComponent.h>
#include <EditorComponents/EditorTerrainWorldRendererComponent.h>
#include <TerrainRenderer/EditorComponents/EditorTerrainSurfaceMaterialsListComponent.h>
#include <TerrainRenderer/EditorComponents/EditorTerrainMacroMaterialComponent.h>

namespace Terrain
{
    EditorTerrainModule::EditorTerrainModule()
    {
        m_descriptors.insert(
            m_descriptors.end(),
            {
                Terrain::EditorTerrainHeightGradientListComponent::CreateDescriptor(),
                Terrain::EditorTerrainLayerSpawnerComponent::CreateDescriptor(),
                Terrain::EditorTerrainMacroMaterialComponent::CreateDescriptor(),
                Terrain::EditorTerrainSurfaceGradientListComponent::CreateDescriptor(),
                Terrain::EditorTerrainSystemComponent::CreateDescriptor(),
                Terrain::EditorTerrainSurfaceMaterialsListComponent::CreateDescriptor(),
                Terrain::EditorTerrainWorldComponent::CreateDescriptor(),
                Terrain::EditorTerrainWorldDebuggerComponent::CreateDescriptor(),
                Terrain::EditorTerrainPhysicsColliderComponent::CreateDescriptor(),
                Terrain::EditorTerrainWorldRendererComponent::CreateDescriptor(),

            });
    }

    AZ::ComponentTypeList EditorTerrainModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = TerrainModule::GetRequiredSystemComponents();
        requiredComponents.insert(
            requiredComponents.end(),
            {
                azrtti_typeid<EditorTerrainSystemComponent>(),
            });

        return requiredComponents;
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), Terrain::EditorTerrainModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Terrain_Editor, Terrain::EditorTerrainModule)
#endif
