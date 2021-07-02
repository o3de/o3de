/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"

#include <GradientSignalModule.h>
#include <GradientSignalSystemComponent.h>
#include <Components/SmoothStepGradientComponent.h>
#include <Components/SurfaceAltitudeGradientComponent.h>
#include <Components/SurfaceSlopeGradientComponent.h>
#include <Components/MixedGradientComponent.h>
#include <Components/ImageGradientComponent.h>
#include <Components/ConstantGradientComponent.h>
#include <Components/ThresholdGradientComponent.h>
#include <Components/LevelsGradientComponent.h>
#include <Components/ReferenceGradientComponent.h>
#include <Components/InvertGradientComponent.h>
#include <Components/DitherGradientComponent.h>
#include <Components/PosterizeGradientComponent.h>
#include <Components/ShapeAreaFalloffGradientComponent.h>
#include <Components/PerlinGradientComponent.h>
#include <Components/RandomGradientComponent.h>
#include <Components/GradientTransformComponent.h>
#include <Components/SurfaceMaskGradientComponent.h>
#include <Components/GradientSurfaceDataComponent.h>

namespace GradientSignal
{
    GradientSignalModule::GradientSignalModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            GradientSignalSystemComponent::CreateDescriptor(),

            SmoothStepGradientComponent::CreateDescriptor(),
            SurfaceAltitudeGradientComponent::CreateDescriptor(),
            SurfaceSlopeGradientComponent::CreateDescriptor(),
            MixedGradientComponent::CreateDescriptor(),
            ImageGradientComponent::CreateDescriptor(),
            ConstantGradientComponent::CreateDescriptor(),
            ThresholdGradientComponent::CreateDescriptor(),
            LevelsGradientComponent::CreateDescriptor(),
            ReferenceGradientComponent::CreateDescriptor(),
            InvertGradientComponent::CreateDescriptor(),
            DitherGradientComponent::CreateDescriptor(),
            PosterizeGradientComponent::CreateDescriptor(),
            ShapeAreaFalloffGradientComponent::CreateDescriptor(),
            PerlinGradientComponent::CreateDescriptor(),
            RandomGradientComponent::CreateDescriptor(),
            GradientTransformComponent::CreateDescriptor(),
            SurfaceMaskGradientComponent::CreateDescriptor(),
            GradientSurfaceDataComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList GradientSignalModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
                azrtti_typeid<GradientSignalSystemComponent>(),
        };
    }
}

#if !defined(GRADIENTSIGNAL_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_GradientSignal, GradientSignal::GradientSignalModule)
#endif
