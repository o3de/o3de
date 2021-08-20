/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorTerrainModule.h>
#include <EditorComponents/EditorTerrainSystemComponent.h>

namespace Terrain
{
    EditorTerrainModule::EditorTerrainModule()
    {
        m_descriptors.insert(
            m_descriptors.end(),
            {
                Terrain::EditorTerrainSystemComponent::CreateDescriptor(),
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
