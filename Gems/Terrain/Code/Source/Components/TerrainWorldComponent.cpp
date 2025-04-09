/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Terrain
{
    
    AZ::JsonSerializationResult::Result JsonTerrainWorldConfigSerializer::Load(
        void* outputValue, [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        auto configInstance = reinterpret_cast<TerrainWorldConfig*>(outputValue);
        AZ_Assert(configInstance, "Output value for JsonTerrainWorldConfigSerializer can't be null.");
        
        JSR::ResultCode result(JSR::Tasks::ReadField);

        auto arrayFloatToSingleValue = [&](const char* oldName, const char* newName, auto& dataRef, uint32_t index)
        {
            rapidjson::Value::ConstMemberIterator itr = inputValue.FindMember(oldName);
            if (itr != inputValue.MemberEnd() && itr->value.IsArray())
            {
                dataRef = itr->value.GetArray()[index].GetFloat();
            }
            else
            {
                result.Combine(ContinueLoadingFromJsonObjectField(
                    &dataRef, azrtti_typeid<decltype(dataRef)>(), inputValue, rapidjson::GenericStringRef<char>(newName), context));
            }
        };

        arrayFloatToSingleValue("WorldMin", "MinHeight", configInstance->m_minHeight, 2);
        arrayFloatToSingleValue("WorldMax", "MaxHeight", configInstance->m_maxHeight, 2);

        arrayFloatToSingleValue("HeightQueryResolution", "HeightQueryResolution", configInstance->m_heightQueryResolution, 0);

        result.Combine(ContinueLoadingFromJsonObjectField(
            &configInstance->m_surfaceDataQueryResolution, azrtti_typeid<decltype(configInstance->m_surfaceDataQueryResolution)>(),
            inputValue, "SurfaceDataQueryResolution", context));

        return context.Report(result,
            result.GetProcessing() != JSR::Processing::Halted ?
            "Successfully loaded TerrainWorldConfig information." :
            "Failed to load TerrainWorldConfig information.");
    }
    
    AZ_CLASS_ALLOCATOR_IMPL(JsonTerrainWorldConfigSerializer, AZ::SystemAllocator);

    void TerrainWorldConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<JsonTerrainWorldConfigSerializer>()->HandlesType<TerrainWorldConfig>();
        }

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainWorldConfig, AZ::ComponentConfig>()
                ->Version(4)
                ->Field("MinHeight", &TerrainWorldConfig::m_minHeight)
                ->Field("MaxHeight", &TerrainWorldConfig::m_maxHeight)
                ->Field("HeightQueryResolution", &TerrainWorldConfig::m_heightQueryResolution)
                ->Field("SurfaceDataQueryResolution", &TerrainWorldConfig::m_surfaceDataQueryResolution)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainWorldConfig>("Terrain World Component", "Data required for the terrain system to run")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_minHeight, "Min Height", "")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, -1000.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1000.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, -65536.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 65536.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeValidate, &TerrainWorldConfig::ValidateHeightMin)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_maxHeight, "Max Height", "")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, -1000.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1000.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, -65536.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 65536.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeValidate, &TerrainWorldConfig::ValidateHeightMax)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_heightQueryResolution, "Height Query Resolution (m)", "")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_surfaceDataQueryResolution, "Surface Data Query Resolution (m)", "")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ;
            }
        }
    }

    void TerrainWorldComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainService"));
    }

    void TerrainWorldComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainService"));
    }

    void TerrainWorldComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void TerrainWorldComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainWorldConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainWorldComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainWorldComponent::m_configuration)
            ;
        }
    }

    TerrainWorldComponent::TerrainWorldComponent(const TerrainWorldConfig& configuration)
        : m_configuration(configuration)
    {
    }

    TerrainWorldComponent::~TerrainWorldComponent()
    {
    }

    void TerrainWorldComponent::Activate()
    {
        // Currently, the Terrain System Component owns the Terrain System instance because the Terrain World component gets recreated
        // every time an entity is added or removed to a level.  If this ever changes, the Terrain System ownership could move into
        // the level component.
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::Activate);

        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequestBus::Events::SetTerrainHeightBounds,
            AzFramework::Terrain::FloatRange({ m_configuration.m_minHeight, m_configuration.m_maxHeight }));
        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequestBus::Events::SetTerrainHeightQueryResolution, m_configuration.m_heightQueryResolution);
        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequestBus::Events::SetTerrainSurfaceDataQueryResolution, m_configuration.m_surfaceDataQueryResolution);
    }

    void TerrainWorldComponent::Deactivate()
    {
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::Deactivate);
    }

    bool TerrainWorldComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainWorldConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainWorldComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainWorldConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

} // namespace Terrain
