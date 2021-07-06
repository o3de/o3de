/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SurfaceData_precompiled.h"

#include <SurfaceDataModule.h>
#include <SurfaceDataSystemComponent.h>
#include <Components/SurfaceDataColliderComponent.h>
#include <Components/SurfaceDataShapeComponent.h>
#include <TerrainSurfaceDataSystemComponent.h>

namespace SurfaceData
{
    SurfaceDataModule::SurfaceDataModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            SurfaceDataSystemComponent::CreateDescriptor(),
            SurfaceDataColliderComponent::CreateDescriptor(),
            SurfaceDataShapeComponent::CreateDescriptor(),
            TerrainSurfaceDataSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList SurfaceDataModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<SurfaceDataSystemComponent>(),
            azrtti_typeid<TerrainSurfaceDataSystemComponent>(),
        };
    }
}

#if !defined(SURFACEDATA_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_SurfaceData, SurfaceData::SurfaceDataModule)
#endif
