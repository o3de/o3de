/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ReflectSystemComponent.h>
#include <RHI.Private/FactoryManagerSystemComponent.h>
#include <RHI.Private/FactoryRegistrationFinalizerSystemComponent.h>
#include <RHI.Profiler/GraphicsProfilerSystemComponent.h>
#include <AzCore/Module/Module.h>

namespace AZ::RHI
{
    //! This module is in charge of loading the RHI reflection descriptor and the
    //! system components in charge of managing the different factory backends.
    class PlatformModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(PlatformModule, "{C34AA64E-0983-4D30-A33C-0D7C7676A20E}", Module);

        PlatformModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                ReflectSystemComponent::CreateDescriptor(),
                FactoryManagerSystemComponent::CreateDescriptor(),
                FactoryRegistrationFinalizerSystemComponent::CreateDescriptor(),
                GraphicsProfilerSystemComponent::CreateDescriptor()
            });
        }
        ~PlatformModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
            {
                azrtti_typeid<FactoryManagerSystemComponent>(),
                azrtti_typeid<FactoryRegistrationFinalizerSystemComponent>()
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Private), AZ::RHI::PlatformModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_Private, AZ::RHI::PlatformModule)
#endif
