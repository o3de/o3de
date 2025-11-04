/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), FastNoiseGem::FastNoiseModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_FastNoise, FastNoiseGem::FastNoiseModule)
#endif
#endif
