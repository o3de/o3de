/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/DX12/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>
#include <AzCore/Module/Module.h>
#include <RHI.Builders/ShaderPlatformInterfaceSystemComponent.h>

namespace AZ
{
    namespace DX12
    {
        /**
         * Exposes DX12 RHI Building components to the Asset Processor.
         */
        class BuilderModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(BuilderModule, "{B4EFEB3F-93A7-4981-BE47-395AFBEBABC6}", AZ::Module);

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
    } // namespace DX12
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Builders), AZ::DX12::BuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_DX12_Builders, AZ::DX12::BuilderModule)
#endif
