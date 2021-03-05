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

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <RHI.Builders/ShaderPlatformInterfaceSystemComponent.h>
#include <RHI.Builders/ShaderPlatformInterface.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/Metal/Base.h>

namespace AZ
{
    namespace Metal
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
            dependent.push_back(AZ_CRC("AzslShaderBuilderService", 0x09315a40));
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

    } // namespace Metal
} // namespace AZ
