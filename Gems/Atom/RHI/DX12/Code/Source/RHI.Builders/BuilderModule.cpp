/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_DX12_Builders, AZ::DX12::BuilderModule);
