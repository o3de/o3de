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

#pragma once

#include <Atom/RHI.Edit/ShaderPlatformInterfaceBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI.Builders/ShaderPlatformInterface.h>

namespace AZ
{
    namespace Null
    {
        class ShaderPlatformInterfaceSystemComponent final
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(ShaderPlatformInterfaceSystemComponent, "{CC4C6744-2D8F-461C-AC86-6C1563BC2552}");
            static void Reflect(AZ::ReflectContext* context);

            ShaderPlatformInterfaceSystemComponent() = default;
            ~ShaderPlatformInterfaceSystemComponent() override = default;

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            // AZ::Component
            void Activate() override;
            void Deactivate() override;

        private:
            ShaderPlatformInterfaceSystemComponent(const ShaderPlatformInterfaceSystemComponent&) = delete;

            AZStd::unique_ptr<ShaderPlatformInterface> m_shaderPlatformInterface;
        };
    } // namespace Null
} // namespace AZ
