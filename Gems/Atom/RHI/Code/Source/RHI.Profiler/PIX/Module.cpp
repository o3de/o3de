/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <RHI.Profiler/PIX/PIXSystemComponent.h>

namespace AZ::RHI
{
    //! This module is in charge of loading the PIX system component
    class PIXModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(PIXModule, "{AE00105A-936F-43E0-82D8-73A1E335E158}", Module);

        PIXModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                PIXSystemComponent::CreateDescriptor()
            });
        }
        ~PIXModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
            {
                azrtti_typeid<PIXSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::RHI::PIXModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_Profiler_PIX, AZ::RHI::PIXModule)
#endif
