/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <TerrainModule.h>

#include <TerrainSystemComponent.h>
#include <TerrainComponent.h>

namespace Terrain
{
    TerrainModule::TerrainModule()
        : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            TerrainSystemComponent::CreateDescriptor(),
                Terrain::TerrainComponent::CreateDescriptor(),
            });
    }

    /**
        * Add required SystemComponents to the SystemEntity.
        */
    AZ::ComponentTypeList TerrainModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList {
            azrtti_typeid<TerrainSystemComponent>(),
        };
    }

}// namespace Terrain

#if !defined(TERRAIN_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Terrain, Terrain::TerrainModule)
#endif
