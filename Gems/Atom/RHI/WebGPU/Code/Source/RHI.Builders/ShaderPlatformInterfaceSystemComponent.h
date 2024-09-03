/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Edit/ShaderPlatformInterfaceBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI.Builders/ShaderPlatformInterface.h>

namespace AZ
{
    namespace WebGPU
    {
        class ShaderPlatformInterfaceSystemComponent final
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(ShaderPlatformInterfaceSystemComponent, "{F77792BB-3090-4C21-8B86-9D6DB99610E6}");
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
    } // namespace WebGPU
} // namespace AZ
