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
#include <EditorComponents/EditorTerrainSystemComponent.h>
#include <EditorComponents/EditorTerrainWorldComponent.h>
#include <EditorComponents/EditorTerrainWorldDebuggerComponent.h>
#include <EditorComponents/EditorTerrainWorldRendererComponent.h>

namespace Terrain
{
    EditorTerrainModule::EditorTerrainModule()
    {
        m_descriptors.insert(
            m_descriptors.end(),
            {
                Terrain::EditorTerrainHeightGradientListComponent::CreateDescriptor(),
                Terrain::EditorTerrainLayerSpawnerComponent::CreateDescriptor(),
                Terrain::EditorTerrainSystemComponent::CreateDescriptor(),
                Terrain::EditorTerrainWorldComponent::CreateDescriptor(),
                Terrain::EditorTerrainWorldDebuggerComponent::CreateDescriptor(),
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

AZ_DECLARE_MODULE_CLASS(Gem_TerrainEditor, Terrain::EditorTerrainModule)
