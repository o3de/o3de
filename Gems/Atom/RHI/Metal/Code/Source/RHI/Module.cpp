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
#include "Atom_RHI_Metal_precompiled.h"

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

AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_Metal_Private, AZ::Metal::PlatformModule)
