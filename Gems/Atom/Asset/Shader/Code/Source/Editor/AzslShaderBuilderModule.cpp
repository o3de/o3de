/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <AzslShaderBuilderSystemComponent.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        class AzslShaderBuilderModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(AzslShaderBuilderModule, "{43370465-DBF1-44BB-968D-97C0B42F5EA0}", AZ::Module);
            AZ_CLASS_ALLOCATOR(AzslShaderBuilderModule, AZ::SystemAllocator);

            AzslShaderBuilderModule()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                m_descriptors.insert(m_descriptors.end(), {
                    AzslShaderBuilderSystemComponent::CreateDescriptor(),
                });
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<AzslShaderBuilderSystemComponent>(),
                };
            }
        };
    } // namespace ShaderBuilder
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Builders), AZ::ShaderBuilder::AzslShaderBuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomShader_Builders, AZ::ShaderBuilder::AzslShaderBuilderModule)
#endif
