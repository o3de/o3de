/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceDataModule.h>
#include <SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <SurfaceData/Components/SurfaceDataColliderComponent.h>
#include <SurfaceData/Components/SurfaceDataShapeComponent.h>

namespace SurfaceData
{
    SurfaceDataModule::SurfaceDataModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            SurfaceDataSystemComponent::CreateDescriptor(),
            SurfaceDataColliderComponent::CreateDescriptor(),
            SurfaceDataShapeComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList SurfaceDataModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<SurfaceDataSystemComponent>(),
        };
    }
}

#if !defined(SURFACEDATA_EDITOR)
#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), SurfaceData::SurfaceDataModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_SurfaceData, SurfaceData::SurfaceDataModule)
#endif
#endif
