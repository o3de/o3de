/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <RHI.Builders/ShaderPlatformInterfaceSystemComponent.h>
#include <RHI.Builders/ShaderPlatformInterface.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/Vulkan/Base.h>

namespace AZ
{
    namespace Vulkan
    {
        void ShaderPlatformInterfaceSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<ShaderPlatformInterfaceSystemComponent, Component>()
                    ->Version(0)
                    ->Attribute(Edit::Attributes::SystemComponentTags, AZStd::vector<Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                    ;
            }
        }

        void ShaderPlatformInterfaceSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("AzslShaderBuilderService"));
        }

        void ShaderPlatformInterfaceSystemComponent::Activate()
        {
            m_shaderPlatformInterface = AZStd::make_unique<ShaderPlatformInterface>(APIUniqueIndex);
            RHI::ShaderPlatformInterfaceRegisterBus::Broadcast(
                &RHI::ShaderPlatformInterfaceRegister::RegisterShaderPlatformHandler,
                m_shaderPlatformInterface.get());
        }

        void ShaderPlatformInterfaceSystemComponent::Deactivate()
        {
            RHI::ShaderPlatformInterfaceRegisterBus::Broadcast(
                &RHI::ShaderPlatformInterfaceRegister::UnregisterShaderPlatformHandler, 
                m_shaderPlatformInterface.get());

            m_shaderPlatformInterface = nullptr;
        }

    } // namespace Vulkan
} // namespace AZ
