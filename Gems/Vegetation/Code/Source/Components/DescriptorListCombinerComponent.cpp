/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DescriptorListCombinerComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/InstanceData.h>
#include <AzCore/Debug/Profiler.h>

#include <VegetationProfiler.h>

namespace Vegetation
{
    void DescriptorListCombinerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DescriptorListCombinerConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("DescriptorProviders", &DescriptorListCombinerConfig::m_descriptorProviders)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<DescriptorListCombinerConfig>(
                    "Vegetation Asset List Combiner", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &DescriptorListCombinerConfig::m_descriptorProviders, "Descriptor Providers", "Ordered list of descriptor providers.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->ElementAttribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("VegetationDescriptorProviderService"));
                ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DescriptorListCombinerConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Method("GetNumDescriptors", &DescriptorListCombinerConfig::GetNumDescriptors)
                ->Method("GetDescriptorEntityId", &DescriptorListCombinerConfig::GetDescriptorEntityId)
                ->Method("RemoveDescriptorEntityId", &DescriptorListCombinerConfig::RemoveDescriptorEntityId)
                ->Method("SetDescriptorEntityId", &DescriptorListCombinerConfig::SetDescriptorEntityId)
                ->Method("AddDescriptorEntityId", &DescriptorListCombinerConfig::AddDescriptorEntityId)
                ;
        }
    }

    size_t DescriptorListCombinerConfig::GetNumDescriptors() const
    {
        return m_descriptorProviders.size();
    }

    AZ::EntityId DescriptorListCombinerConfig::GetDescriptorEntityId(int index) const
    {
        if (index < m_descriptorProviders.size() && index >= 0)
        {
            return m_descriptorProviders[index];
        }

        return AZ::EntityId();
    }

    void DescriptorListCombinerConfig::RemoveDescriptorEntityId(int index)
    {
        if (index < m_descriptorProviders.size() && index >= 0)
        {
            m_descriptorProviders.erase(m_descriptorProviders.begin() + index);
        }

    }

    void DescriptorListCombinerConfig::SetDescriptorEntityId(int index, AZ::EntityId entityId)
    {
        if (index < m_descriptorProviders.size() && index >= 0)
        {
            m_descriptorProviders[index] = entityId;
        }

    }

    void DescriptorListCombinerConfig::AddDescriptorEntityId(AZ::EntityId entityId)
    {
        m_descriptorProviders.push_back(entityId);
    }

    void DescriptorListCombinerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationDescriptorProviderService"));
    }

    void DescriptorListCombinerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationDescriptorProviderService"));
    }

    void DescriptorListCombinerComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void DescriptorListCombinerComponent::Reflect(AZ::ReflectContext* context)
    {
        DescriptorListCombinerConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DescriptorListCombinerComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &DescriptorListCombinerComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("DescriptorListCombinerComponentTypeId", BehaviorConstant(DescriptorListCombinerComponentTypeId));

            behaviorContext->Class<DescriptorListCombinerComponent>()->RequestBus("DescriptorListCombinerRequestBus");

            behaviorContext->EBus<DescriptorListCombinerRequestBus>("DescriptorListCombinerRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetNumDescriptors", &DescriptorListCombinerRequestBus::Events::GetNumDescriptors)
                ->Event("GetDescriptorEntityId", &DescriptorListCombinerRequestBus::Events::GetDescriptorEntityId)
                ->Event("RemoveDescriptorEntityId", &DescriptorListCombinerRequestBus::Events::RemoveDescriptorEntityId)
                ->Event("SetDescriptorEntityId", &DescriptorListCombinerRequestBus::Events::SetDescriptorEntityId)
                ->Event("AddDescriptorEntityId", &DescriptorListCombinerRequestBus::Events::AddDescriptorEntityId)
                ;
        }
    }

    DescriptorListCombinerComponent::DescriptorListCombinerComponent(const DescriptorListCombinerConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void DescriptorListCombinerComponent::SetupDependencies()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependencies(m_configuration.m_descriptorProviders);
    }

    void DescriptorListCombinerComponent::Activate()
    {
        SetupDependencies();
        DescriptorProviderRequestBus::Handler::BusConnect(GetEntityId());
        DescriptorListCombinerRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DescriptorListCombinerComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        DescriptorProviderRequestBus::Handler::BusDisconnect();
        DescriptorListCombinerRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler::BusDisconnect();
    }

    bool DescriptorListCombinerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const DescriptorListCombinerConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool DescriptorListCombinerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<DescriptorListCombinerConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void DescriptorListCombinerComponent::GetDescriptors(DescriptorPtrVec& descriptors) const
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        for (const auto& entityId : m_configuration.m_descriptorProviders)
        {
            if (entityId != GetEntityId())
            {
                DescriptorProviderRequestBus::Event(entityId, &DescriptorProviderRequestBus::Events::GetDescriptors, descriptors);
            }
        }
    }

    void DescriptorListCombinerComponent::GetInclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags, bool& includeAll) const
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        for (const auto& entityId : m_configuration.m_descriptorProviders)
        {
            if (entityId != GetEntityId())
            {
                SurfaceData::SurfaceDataTagEnumeratorRequestBus::Event(entityId, &SurfaceData::SurfaceDataTagEnumeratorRequestBus::Events::GetInclusionSurfaceTags, tags, includeAll);
            }
        }
    }

    void DescriptorListCombinerComponent::GetExclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags) const
    {
        AZ_PROFILE_FUNCTION(Vegetation);

        for (const auto& entityId : m_configuration.m_descriptorProviders)
        {
            if (entityId != GetEntityId())
            {
                SurfaceData::SurfaceDataTagEnumeratorRequestBus::Event(entityId, &SurfaceData::SurfaceDataTagEnumeratorRequestBus::Events::GetExclusionSurfaceTags, tags);
            }
        }
    }

    size_t DescriptorListCombinerComponent::GetNumDescriptors() const
    {
        return m_configuration.GetNumDescriptors();
    }

    AZ::EntityId DescriptorListCombinerComponent::GetDescriptorEntityId(int index) const
    {
        return m_configuration.GetDescriptorEntityId(index);
    }

    void DescriptorListCombinerComponent::RemoveDescriptorEntityId(int index)
    {
        m_configuration.RemoveDescriptorEntityId(index);
        SetupDependencies();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void DescriptorListCombinerComponent::SetDescriptorEntityId(int index, AZ::EntityId entityId)
    {
        m_configuration.SetDescriptorEntityId(index, entityId);
        SetupDependencies();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void DescriptorListCombinerComponent::AddDescriptorEntityId(AZ::EntityId entityId)
    {
        m_configuration.AddDescriptorEntityId(entityId);
        SetupDependencies();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
