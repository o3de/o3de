/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <AzCore/Module/Module.h>
#include <Source/RHI/SystemComponent.h>
#include <Atom/RHI.Reflect/WebGPU/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

namespace AZ
{
    namespace WebGPU
    {
        class PlatformModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(PlatformModule, "{824BAE2F-338F-4775-B330-0BBAB2D0FADC}", Module);

            PlatformModule()
            {
                m_descriptors.insert(m_descriptors.end(), {
                    ReflectSystemComponent::CreateDescriptor(),
                    SystemComponent::CreateDescriptor()
                });
            }
            ~PlatformModule() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return
                {
                    azrtti_typeid<WebGPU::SystemComponent>()
                };
            }
        };
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Private), AZ::WebGPU::PlatformModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_WebGPU_Private, AZ::WebGPU::PlatformModule)
#endif
