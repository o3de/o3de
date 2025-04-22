/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <GradientSignalModule.h>
#include <GradientSignalSystemComponent.h>
#include <GradientSignal/Components/SmoothStepGradientComponent.h>
#include <GradientSignal/Components/SurfaceAltitudeGradientComponent.h>
#include <GradientSignal/Components/SurfaceSlopeGradientComponent.h>
#include <GradientSignal/Components/MixedGradientComponent.h>
#include <GradientSignal/Components/ImageGradientComponent.h>
#include <GradientSignal/Components/ConstantGradientComponent.h>
#include <GradientSignal/Components/ThresholdGradientComponent.h>
#include <GradientSignal/Components/LevelsGradientComponent.h>
#include <GradientSignal/Components/ReferenceGradientComponent.h>
#include <GradientSignal/Components/InvertGradientComponent.h>
#include <GradientSignal/Components/DitherGradientComponent.h>
#include <GradientSignal/Components/PosterizeGradientComponent.h>
#include <GradientSignal/Components/ShapeAreaFalloffGradientComponent.h>
#include <GradientSignal/Components/PerlinGradientComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>
#include <GradientSignal/Components/SurfaceMaskGradientComponent.h>
#include <GradientSignal/Components/GradientSurfaceDataComponent.h>

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
#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), GradientSignal::GradientSignalModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_GradientSignal, GradientSignal::GradientSignalModule)
#endif
#endif
