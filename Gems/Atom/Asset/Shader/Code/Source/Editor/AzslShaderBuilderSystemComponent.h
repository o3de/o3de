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

#include <AzCore/Component/Component.h>

#include <Atom/RHI.Edit/ShaderPlatformInterfaceBus.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include "AzslBuilder.h"
#include "SrgLayoutBuilder.h"
#include "ShaderAssetBuilder.h"
#include "ShaderVariantAssetBuilder.h"
#include "PrecompiledShaderBuilder.h"
#include "ShaderPlatformInterfaceRequest.h"
#include "ShaderAssetBuilder2.h"
#include "ShaderVariantAssetBuilder2.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        
        class AzslShaderBuilderSystemComponent
            : public Component
            , public RHI::ShaderPlatformInterfaceRegisterBus::Handler
            , public ShaderPlatformInterfaceRequestBus::Handler
        {
        public:
            AZ_COMPONENT(AzslShaderBuilderSystemComponent, "{56B5B944-8AF4-4478-A047-8DFDE38DA681}");

            static void Reflect(ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            ////////////////////////////////////////////////////////////////////////
            // AzslShaderBuilderRequestBus interface implementation

            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            // ShaderPlatformInterfaceRegisterBus interface overrides ...
            void RegisterShaderPlatformHandler(RHI::ShaderPlatformInterface* shaderPlatformInterface) override;
            void UnregisterShaderPlatformHandler(RHI::ShaderPlatformInterface* shaderPlatformInterface) override;

            // ShaderPlatformInterfaceRequestBus interface overrides ...
            AZStd::vector<RHI::ShaderPlatformInterface*> GetShaderPlatformInterface(const AssetBuilderSDK::PlatformInfo& platformInfo) override;

        private:
            AzslBuilder m_azslBuilder;
            SrgLayoutBuilder m_srgLayoutBuilder;
            ShaderAssetBuilder m_shaderAssetBuilder;
            ShaderVariantAssetBuilder m_shaderVariantAssetBuilder;
            PrecompiledShaderBuilder m_precompiledShaderBuilder;
            ShaderAssetBuilder2 m_shaderAssetBuilder2;
            ShaderVariantAssetBuilder2 m_shaderVariantAssetBuilder2;

            /// Contains the ShaderPlatformInterface for all registered RHIs
            AZStd::unordered_map<RHI::APIType, RHI::ShaderPlatformInterface*> m_shaderPlatformInterfaces;
        };
        
    } // ShaderBuilder
} //AZ
