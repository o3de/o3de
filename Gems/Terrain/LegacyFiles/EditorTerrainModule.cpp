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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <TerrainModule.h>
#include <TerrainSystemComponent.h>
#include <Components/EditorTerrainWorldComponent.h>
#include <Components/EditorTerrainHeightGradientListComponent.h>
#include <Components/EditorTerrainLayerSpawnerComponent.h>

namespace Terrain
{
    class EditorTerrainModule
        : public TerrainModule
    {
    public:
        AZ_RTTI(EditorTerrainModule, "{C47C54CC-7B72-4159-BB40-ABE058A4A978}", AZ::Module);
        AZ_CLASS_ALLOCATOR(EditorTerrainModule, AZ::SystemAllocator, 0);

        EditorTerrainModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                    EditorTerrainWorldComponent::CreateDescriptor(),
                    EditorTerrainHeightGradientListComponent::CreateDescriptor(),
                    EditorTerrainLayerSpawnerComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList requiredComponents = TerrainModule::GetRequiredSystemComponents();

            return requiredComponents;
        }
    };
}

AZ_DECLARE_MODULE_CLASS(EditorTerrain, Terrain::EditorTerrainModule)
