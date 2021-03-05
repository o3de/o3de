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

#include "SurfaceData_precompiled.h"

#include <SurfaceDataModule.h>
#include <SurfaceDataSystemComponent.h>
#include <Components/SurfaceDataColliderComponent.h>
#include <Components/SurfaceDataMeshComponent.h>
#include <Components/SurfaceDataShapeComponent.h>
#include <TerrainSurfaceDataSystemComponent.h>

namespace SurfaceData
{
    SurfaceDataModule::SurfaceDataModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            SurfaceDataSystemComponent::CreateDescriptor(),
            SurfaceDataColliderComponent::CreateDescriptor(),
            SurfaceDataMeshComponent::CreateDescriptor(),
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
