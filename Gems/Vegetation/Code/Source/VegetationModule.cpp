/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationModule.h>

#include <Components/AreaBlenderComponent.h>
#include <Components/BlockerComponent.h>
#include <Components/DescriptorListCombinerComponent.h>
#include <Components/DescriptorListComponent.h>
#include <Components/DescriptorWeightSelectorComponent.h>
#include <Components/DistanceBetweenFilterComponent.h>
#include <Components/DistributionFilterComponent.h>
#include <Components/LevelSettingsComponent.h>
#include <Components/MeshBlockerComponent.h>
#include <Components/PositionModifierComponent.h>
#include <Components/RotationModifierComponent.h>
#include <Components/ScaleModifierComponent.h>
#include <Components/ShapeIntersectionFilterComponent.h>
#include <Components/SlopeAlignmentModifierComponent.h>
#include <Components/SpawnerComponent.h>
#include <Components/SurfaceAltitudeFilterComponent.h>
#include <Components/SurfaceMaskDepthFilterComponent.h>
#include <Components/SurfaceMaskFilterComponent.h>
#include <Components/SurfaceSlopeFilterComponent.h>
#include <Debugger/DebugComponent.h>
#include <Debugger/AreaDebugComponent.h>
#include <AreaSystemComponent.h>
#include <InstanceSystemComponent.h>
#include <VegetationSystemComponent.h>
#include <DebugSystemComponent.h>

namespace Vegetation
{
    VegetationModule::VegetationModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {

            AreaBlenderComponent::CreateDescriptor(),
            BlockerComponent::CreateDescriptor(),
            DescriptorListCombinerComponent::CreateDescriptor(),
            DescriptorListComponent::CreateDescriptor(),
            DescriptorWeightSelectorComponent::CreateDescriptor(),
            DistanceBetweenFilterComponent::CreateDescriptor(),
            DistributionFilterComponent::CreateDescriptor(),
            LevelSettingsComponent::CreateDescriptor(),
            MeshBlockerComponent::CreateDescriptor(),
            PositionModifierComponent::CreateDescriptor(),
            RotationModifierComponent::CreateDescriptor(),
            ScaleModifierComponent::CreateDescriptor(),
            ShapeIntersectionFilterComponent::CreateDescriptor(),
            SlopeAlignmentModifierComponent::CreateDescriptor(),
            SpawnerComponent::CreateDescriptor(),
            SurfaceAltitudeFilterComponent::CreateDescriptor(),
            SurfaceMaskDepthFilterComponent::CreateDescriptor(),
            SurfaceMaskFilterComponent::CreateDescriptor(),
            SurfaceSlopeFilterComponent::CreateDescriptor(),
            AreaSystemComponent::CreateDescriptor(),
            InstanceSystemComponent::CreateDescriptor(),
            VegetationSystemComponent::CreateDescriptor(),
            DebugComponent::CreateDescriptor(),
            DebugSystemComponent::CreateDescriptor(),
            AreaDebugComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList VegetationModule::GetRequiredSystemComponents() const
    {
        // [LY-90913] Revisit the need for these to be required components if/when other components ever get created that fulfill the same
        // service and interface as these.  Until then, making them required improves usability because users will be guided to add all the 
        // dependent system components that vegetation needs.
        return AZ::ComponentTypeList{
            azrtti_typeid<VegetationSystemComponent>(),
            azrtti_typeid<AreaSystemComponent>(),
            azrtti_typeid<InstanceSystemComponent>(),
            azrtti_typeid<DebugSystemComponent>(),
        };
    }
}

#if !defined(VEGETATION_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Vegetation, Vegetation::VegetationModule)
#endif
