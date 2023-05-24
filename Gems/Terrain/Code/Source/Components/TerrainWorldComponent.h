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
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <TerrainSystem/TerrainSystem.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    // Custom JSON serializer for TerrainWorldConfig to handle version conversion
    class JsonTerrainWorldConfigSerializer : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(Terrain::JsonTerrainWorldConfigSerializer, "{910BC31F-CD49-488E-8004-227D9FEB5A16}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(
            void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;
    };

    class TerrainWorldConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainWorldConfig, AZ::SystemAllocator);
        AZ_RTTI(TerrainWorldConfig, "{295844DB-20DD-45B2-94DB-4245D5AE9AFF}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        float m_minHeight{ 0.0f };
        float m_maxHeight{ 1024.0f };
        float m_heightQueryResolution{ 1.0f };
        float m_surfaceDataQueryResolution{ 1.0f };

        static AZ::Outcome<void, AZStd::string> ValidateHeight(float minHeight, float maxHeight)
        {
            if (minHeight > maxHeight)
            {
                return AZ::Failure(AZStd::string("Terrain min height must be less than max height."));
            }
            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> ValidateHeightMin(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            return ValidateHeight(*static_cast<float*>(newValue), m_maxHeight);
        }

        AZ::Outcome<void, AZStd::string> ValidateHeightMax(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            return ValidateHeight(m_minHeight, *static_cast<float*>(newValue));
        }
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
