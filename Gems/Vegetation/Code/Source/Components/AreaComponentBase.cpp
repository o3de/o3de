/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Vegetation/AreaComponentBase.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <AzCore/Debug/Profiler.h>

#include <VegetationProfiler.h>

namespace Vegetation
{
    namespace AreaUtil
    {
        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 1)
            {
                AZ::u8 areaType = 0; //default to cluster
                if (classElement.GetChildData(AZ_CRC_CE("AreaType"), areaType))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("AreaType"));
                    switch (areaType)
                    {
                    default:
                    case 0: //cluster
                        classElement.AddElementWithData(context, "Layer", AreaConstants::s_foregroundLayer);
                        break;
                    case 1: //coverage
                        classElement.AddElementWithData(context, "Layer", AreaConstants::s_backgroundLayer);
                        break;
                    }
                }

                int priority = 1;
                if (classElement.GetChildData(AZ_CRC_CE("Priority"), priority))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("Priority"));
                    classElement.AddElementWithData(context, "Priority", (float)(priority - 1) / (float)std::numeric_limits<int>::max());
                }
            }
            if (classElement.GetVersion() < 2)
            {
                float priority = 0.0f;
                if (classElement.GetChildData(AZ_CRC_CE("Priority"), priority))
                {
                    priority = AZ::GetClamp(priority, 0.0f, 1.0f);
                    const AZ::u32 convertedPriority = (AZ::u32)(priority * AreaConstants::s_prioritySoftMax); //using soft max accommodate slider range and int/float conversion
                    classElement.RemoveElementByName(AZ_CRC_CE("Priority"));
                    classElement.AddElementWithData(context, "Priority", convertedPriority);
                }
            }
            return true;
        }
    }

    void AreaConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaConfig, AZ::ComponentConfig>()
                ->Version(2, &AreaUtil::UpdateVersion)
                ->Field("Layer", &AreaConfig::m_layer)
                ->Field("Priority", &AreaConfig::m_priority)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<AreaConfig>(
                    "Vegetation Area", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AreaConfig::m_layer, "Layer Priority", "Defines a high level order vegetation areas are applied")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &AreaConfig::GetSelectableLayers)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &AreaConfig::m_priority, "Sub Priority", "Defines order vegetation areas are applied within a layer.  Larger numbers = higher priority")
                    ->Attribute(AZ::Edit::Attributes::Min, AreaConstants::s_priorityMin)
                    ->Attribute(AZ::Edit::Attributes::Max, AreaConstants::s_priorityMax)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, AreaConstants::s_priorityMin)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, AreaConstants::s_prioritySoftMax)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AreaConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("areaPriority", BehaviorValueProperty(&AreaConfig::m_priority))
                ->Property("areaLayer",
                    [](AreaConfig* config) { return config->m_layer; },
                    [](AreaConfig* config, const AZ::u32& i) { config->m_layer = i; })
                ;
        }
    }

    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> AreaConfig::GetSelectableLayers() const
    {
        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> selectableLayers;
        selectableLayers.push_back({ AreaConstants::s_backgroundLayer, AZStd::string("Background") });
        selectableLayers.push_back({ AreaConstants::s_foregroundLayer, AZStd::string("Foreground") });
        return selectableLayers;
    }

    void AreaComponentBase::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void AreaComponentBase::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
        services.push_back(AZ_CRC_CE("GradientService"));
        services.push_back(AZ_CRC_CE("GradientTransformService"));
    }

    void AreaComponentBase::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void AreaComponentBase::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaComponentBase, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &AreaComponentBase::m_configuration)
                ;
        }
    }

    AreaComponentBase::AreaComponentBase(const AreaConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void AreaComponentBase::Activate()
    {
        m_areaRegistered = false;
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AreaNotificationBus::Handler::BusConnect(GetEntityId());
        AreaInfoBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());

        UpdateRegistration();
    }

    void AreaComponentBase::Deactivate()
    {
        // Disconnect from the busses *before* unregistering to ensure that unregistration can't trigger any
        // messages back into this component while it is deactivating.
        // Specifically, unregistering the area first previously caused a bug in the SpawnerComponent in which OnUnregisterArea
        // cleared out Descriptor pointers, and if any of them went to a refcount of 0, they could trigger an
        // OnCompositionChanged event which ended up looping back into this component.
        AreaNotificationBus::Handler::BusDisconnect();
        AreaInfoBus::Handler::BusDisconnect();
        AreaRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        if (m_areaRegistered)
        {
            m_areaRegistered = false;
            AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::UnregisterArea, GetEntityId());

            // Let area subclasses know that we've just unregistered the area
            OnUnregisterArea();
        }
    }

    bool AreaComponentBase::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AreaConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool AreaComponentBase::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AreaConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    AZ::u32 AreaComponentBase::GetLayer() const
    {
        return m_configuration.m_layer;
    }

    AZ::u32 AreaComponentBase::GetPriority() const
    {
        return m_configuration.m_priority;
    }

    AZ::u32 AreaComponentBase::GetChangeIndex() const
    {
        return m_changeIndex;
    }

    void AreaComponentBase::UpdateRegistration()
    {
        // Area "valid" lifetimes can be shorter than the time in which the area components are active.
        // This can occur due to the chain of entity dependencies, or dependencies on asset loading, etc.
        // This method ensures that we update our registration status so that the area is only registered with
        // the vegetation system once the area is completely valid, and the area is unregistered the moment
        // it becomes invalid.  Right now, we're defining "completely valid" as "has a well-defined valid AABB",
        // since that's the minimum requirement for a vegetation area.

        AZ::u32 layer = GetLayer();
        AZ::u32 priority = GetPriority();
        AZ::Aabb bounds = GetEncompassingAabb();
        bool areaIsValid = bounds.IsValid();

        if (m_areaRegistered && areaIsValid)
        {
            // Area is already registered, we're just updating information, so Refresh the area
            AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RefreshArea, GetEntityId(), layer, priority, bounds);
        }
        else if (!m_areaRegistered && areaIsValid)
        {
            // We've gone from an invalid to valid state, so Register the area
            m_areaRegistered = true;
            AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RegisterArea, GetEntityId(), layer, priority, bounds);

            // Let area subclasses know that we've just registered the area
            OnRegisterArea();
        }
        else if (m_areaRegistered && !areaIsValid)
        {
            // We've gone from a valid to invalid state, so Unregister the area
            m_areaRegistered = false;
            AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::UnregisterArea, GetEntityId());

            // Let area subclasses know that we've just unregistered the area
            OnUnregisterArea();
        }
        else
        {
            // Our state before and after were both invalid, so do nothing.
        }
    }

    void AreaComponentBase::OnCompositionChanged()
    {
        UpdateRegistration();
        ++m_changeIndex;
    }

    void AreaComponentBase::OnAreaConnect()
    {
        AreaRequestBus::Handler::BusConnect(GetEntityId());
    }

    void AreaComponentBase::OnAreaDisconnect()
    {
        AreaRequestBus::Handler::BusDisconnect();
    }

    void AreaComponentBase::OnAreaRefreshed()
    {
    }

    void AreaComponentBase::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        AZ_PROFILE_FUNCTION(Vegetation);
        OnCompositionChanged();
    }

    void AreaComponentBase::OnShapeChanged([[maybe_unused]] ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        AZ_PROFILE_FUNCTION(Vegetation);
        OnCompositionChanged();
    }
}
