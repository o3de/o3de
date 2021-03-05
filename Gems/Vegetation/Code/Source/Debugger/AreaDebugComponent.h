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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/PlatformDef.h>
#include <Vegetation/Ebuses/AreaDebugBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    AZ_INLINE AZ::Color GetDebugColor()
    {
        static uint32 debugColor = 0xff << 8;
        AZ::Color value;
        value.FromU32(debugColor | (0xff << 24)); // add in alpha 255
        // use a golden ratio sequence to generate the next color
        // new color = fract(old color * 1.6)
        // Treat the 24 bits as normalized 0 - 1
        debugColor = (uint32)((((uint64)debugColor * 0x1999999ull) - 0xffffffull) & 0xffffffull);
        return value;
    }

    class AreaDebugConfig : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(AreaDebugConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(AreaDebugConfig, "{A504D6DA-2825-4A0E-A65E-3FC76FC8AFAC}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::Color m_debugColor = GetDebugColor();
        float m_debugCubeSize = 0.25f;
        bool m_hideDebug = false;
    };


    class AreaDebugComponent
        : public AZ::Component
        , private AreaDebugBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(AreaDebugComponent, "{FEF676D4-BC1C-428E-BC9A-C85CF6CF19A5}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        AreaDebugComponent(const AreaDebugConfig& configuration);
        AreaDebugComponent() = default;
        ~AreaDebugComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // AreaDebugBus impl
        AreaDebugDisplayData GetBaseDebugDisplayData() const override;
        void ResetBlendedDebugDisplayData() override;
        void AddBlendedDebugDisplayData(const AreaDebugDisplayData& data) override;
        AreaDebugDisplayData GetBlendedDebugDisplayData() const override;

        bool m_hasBlendedDebugDisplayData = false;
        AreaDebugDisplayData m_blendedDebugDisplayData;

        AreaDebugConfig m_configuration;
    };
}