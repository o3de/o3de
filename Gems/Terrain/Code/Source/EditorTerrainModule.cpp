/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorTerrainModule.h>

namespace Terrain
{
    EditorTerrainModule::EditorTerrainModule()
    {
        m_descriptors.insert(
            m_descriptors.end(),
            {

            });
    }

    AZ::ComponentTypeList EditorTerrainModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = TerrainModule::GetRequiredSystemComponents();
        return requiredComponents;
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_TerrainEditor, Terrain::EditorTerrainModule)
