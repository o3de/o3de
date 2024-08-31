/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/WebGPU/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>
#include <AzCore/Module/Module.h>
#include <RHI.Builders/ShaderPlatformInterfaceSystemComponent.h>

namespace AZ
{
    namespace WebGPU
    {
        //! Exposes WebGPU RHI Building components to the Asset Processor.
        class BuilderModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(BuilderModule, "{09E0CF45-6DCA-4628-8629-E53B5C8BA6BA}", AZ::Module);

            BuilderModule()
            {
                m_descriptors.insert(m_descriptors.end(), {
                    ShaderPlatformInterfaceSystemComponent::CreateDescriptor()
                });
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return
                {
                    azrtti_typeid<ShaderPlatformInterfaceSystemComponent>(),
                };
            }
        };
    } // namespace WebGPU
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Builders), AZ::WebGPU::BuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_WebGPU_Builders, AZ::WebGPU::BuilderModule)
#endif
