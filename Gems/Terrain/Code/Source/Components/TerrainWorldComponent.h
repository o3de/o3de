/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <TerrainSystem/TerrainSystem.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainWorldConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainWorldConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainWorldConfig, "{295844DB-20DD-45B2-94DB-4245D5AE9AFF}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::Vector3 m_worldMin{ 0.0f, 0.0f, 0.0f };
        AZ::Vector3 m_worldMax{ 1024.0f, 1024.0f, 1024.0f };
        AZ::Vector2 m_heightQueryResolution{ 1.0f, 1.0f };

    private:
        AZ::Outcome<void, AZStd::string> ValidateWorldMin(void* newValue, const AZ::Uuid& valueType);
        AZ::Outcome<void, AZStd::string> ValidateWorldMax(void* newValue, const AZ::Uuid& valueType);
        AZ::Outcome<void, AZStd::string> ValidateWorldHeight(void* newValue, const AZ::Uuid& valueType);
        float NumberOfSamples(AZ::Vector3* min, AZ::Vector3* max, AZ::Vector2* heightQuery);
        AZ::Outcome<void, AZStd::string> DetermineMessage(float numSamples);

    };


    class TerrainWorldComponent
        : public AZ::Component
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainWorldComponent, "{4734EFDC-135D-4BF5-BE57-4F9AD03ADF78}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainWorldComponent(const TerrainWorldConfig& configuration);
        TerrainWorldComponent() = default;
        ~TerrainWorldComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    private:
        TerrainWorldConfig m_configuration;
    };
}
