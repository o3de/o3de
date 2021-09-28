/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainSurfaceGradientListComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>

namespace Terrain
{
    void TerrainSurfaceGradientMapping::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainSurfaceGradientMapping>()
                ->Version(1)
                ->Field("Gradient Entity", &TerrainSurfaceGradientMapping::m_gradientEntityId)
                ->Field("Surface Tag", &TerrainSurfaceGradientMapping::m_surfaceTag)
            ;

            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<TerrainSurfaceGradientMapping>("Terrain Surface Gradient Mapping", "Mapping between a gradient and a surface.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientMapping::m_gradientEntityId, "Gradient Entity", "ID of Entity providing a gradient.")
                       ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->UIElement("GradientPreviewer", "Previewer")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientEntity"), &TerrainSurfaceGradientMapping::m_gradientEntityId)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientMapping::m_surfaceTag, "Surface Tag",
                        "Surface type to map to this gradient.")
                ;
            }
        }
    }

    void TerrainSurfaceGradientListConfig::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceGradientMapping::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceGradientListConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Mappings", &TerrainSurfaceGradientListConfig::m_gradientSurfaceMappings)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainSurfaceGradientListConfig>(
                    "Terrain Surface Gradient List Component", "Provide mapping between gradients and surfaces.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientListConfig::m_gradientSurfaceMappings, "Gradient to Surface Mappings", "Maps Gradient Entities to Surfaces.")
                    ;
            }
        }
    }

    void TerrainSurfaceGradientListComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainSurfaceGradientService"));
    }

    void TerrainSurfaceGradientListComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainSurfaceGradientService"));
    }

    void TerrainSurfaceGradientListComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainAreaService"));
    }

    void TerrainSurfaceGradientListComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceGradientListConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceGradientListComponent, AZ::Component>()
                ->Version(0)->Field("Configuration", &TerrainSurfaceGradientListComponent::m_configuration)
            ;
        }
    }

    TerrainSurfaceGradientListComponent::TerrainSurfaceGradientListComponent(const TerrainSurfaceGradientListConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainSurfaceGradientListComponent::Activate()
    {
        Terrain::TerrainGradientSurfaceListServiceRequestBus::Handler::BusConnect(GetEntityId());
    }

    void TerrainSurfaceGradientListComponent::Deactivate()
    {
        Terrain::TerrainGradientSurfaceListServiceRequestBus::Handler::BusDisconnect();
    }

    bool TerrainSurfaceGradientListComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainSurfaceGradientListConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainSurfaceGradientListComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainSurfaceGradientListConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }
    
    void TerrainSurfaceGradientListComponent::GetSurfaceWeights(const AZ::Vector3& inPosition, SurfaceData::SurfaceTagWeightMap& outSurfaceWeights) const
    {
        outSurfaceWeights.clear();

        GradientSignal::GradientSampleParams params(AZ::Vector3(inPosition.GetX(), inPosition.GetY(), 0.0f));

        for (const auto& mapping : m_configuration.m_gradientSurfaceMappings)
        {
            float weight = 0.0f;
            GradientSignal::GradientRequestBus::EventResult(weight, mapping.m_gradientEntityId, &GradientSignal::GradientRequestBus::Events::GetValue, params);

            outSurfaceWeights[mapping.m_surfaceTag] = weight;
        }
    }
} // namespace Terrain
