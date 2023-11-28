/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Metal/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>
#include <AzCore/Module/Module.h>
#include <Source/RHI/SystemComponent.h>

namespace AZ
{
    namespace Metal
    {
        class PlatformModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(PlatformModule, "{75CC9895-51B7-46ED-8C6F-447D77A41B1C}", Module);

            PlatformModule()
            {
                m_descriptors.insert(m_descriptors.end(), {
                    Metal::ReflectSystemComponent::CreateDescriptor(),
                    Metal::SystemComponent::CreateDescriptor()
                });
            }
            ~PlatformModule() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return
                {
                    azrtti_typeid<Metal::SystemComponent>()
                };
            }
        };
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Private), AZ::Metal::PlatformModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_Metal_Private, AZ::Metal::PlatformModule)
#endif
