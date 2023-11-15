/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <RHI/SystemComponent.h>
#include <Atom/RHI.Reflect/DX12/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>
#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace DX12
    {
        class PlatformModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(PlatformModule, "{A5C04CF5-715E-4456-ABF3-A8DB30868D77}", AZ::Module);

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
                    azrtti_typeid<DX12::SystemComponent>()
                };
            }
        };
    } // namespace RHI
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Private), AZ::DX12::PlatformModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_DX12_Private, AZ::DX12::PlatformModule)
#endif
