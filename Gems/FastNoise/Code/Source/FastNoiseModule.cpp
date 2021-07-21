/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FastNoise_precompiled.h"

#include <FastNoiseModule.h>
#include <FastNoiseSystemComponent.h>
#include <FastNoiseGradientComponent.h>

namespace FastNoiseGem
{
    FastNoiseModule::FastNoiseModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            FastNoiseSystemComponent::CreateDescriptor(),
            FastNoiseGradientComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList FastNoiseModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<FastNoiseSystemComponent>(),
        };
    }
}

#if !defined(FASTNOISE_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_FastNoise, FastNoiseGem::FastNoiseModule)
#endif
