/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainEditorModule.h>
#include <EditorTerrainHeightGradientListComponent.h>
#include <EditorTerrainLayerSpawnerComponent.h>
#include <EditorTerrainWorldComponent.h>

namespace Terrain
{
    TerrainEditorModule::TerrainEditorModule()
    {
        m_descriptors.insert(
            m_descriptors.end(),
            {
                Terrain::EditorTerrainHeightGradientListComponent::CreateDescriptor(),
                Terrain::EditorTerrainLayerSpawnerComponent::CreateDescriptor(),
                Terrain::EditorTerrainWorldComponent::CreateDescriptor(),

            });
    }

    AZ::ComponentTypeList TerrainEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = TerrainModule::GetRequiredSystemComponents();
        return requiredComponents;
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_TerrainEditor, Terrain::TerrainEditorModule)
