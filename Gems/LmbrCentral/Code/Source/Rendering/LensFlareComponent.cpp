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
#include "LmbrCentral_precompiled.h"

#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "LensFlareComponent.h"

namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////
    // BehaviorContext LensFlareComponentNotificationBus forwarder
    class BehaviorLensFlareComponentNotificationBusHandler : public LensFlareComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorLensFlareComponentNotificationBusHandler, "{285A6AB7-82CF-472F-8C3B-1659A59213FF}", AZ::SystemAllocator,
            LensFlareTurnedOn, LensFlareTurnedOff);

        void LensFlareTurnedOn()
        {
            Call(FN_LensFlareTurnedOn);
        }

        void LensFlareTurnedOff()
        {
            Call(FN_LensFlareTurnedOff);
        }
    };

    // Class to encapsulate LensFlareComponentRequests::State values.
    class BehaviorLensFlareComponentState
    {
    public:
        AZ_TYPE_INFO(BehaviorLensFlareComponentState, "{1A63ED6B-C2D2-4D3C-AC27-5FDBF22B5B38}");
        AZ_CLASS_ALLOCATOR(BehaviorLensFlareComponentState, AZ::SystemAllocator, 0);
    };

    void LensFlareComponent::Reflect(AZ::ReflectContext* context)
    {
        LensFlareConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<LensFlareComponent, AZ::Component>()->
                Version(1)->
                Field("LensFlareConfiguration", &LensFlareComponent::m_configuration);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<BehaviorLensFlareComponentState>("LensFlareComponentState")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constant("Off", BehaviorConstant(LensFlareComponentRequests::State::Off))
                ->Constant("On", BehaviorConstant(LensFlareComponentRequests::State::On))
                ;

            behaviorContext->EBus<LensFlareComponentRequestBus>("LensFlareComponentRequestBus")
                ->Event("SetLensFlareState", &LensFlareComponentRequestBus::Events::SetLensFlareState)
                ->Event("TurnOnLensFlare", &LensFlareComponentRequestBus::Events::TurnOnLensFlare)
                ->Event("TurnOffLensFlare", &LensFlareComponentRequestBus::Events::TurnOffLensFlare)
                ->Event("ToggleLensFlare", &LensFlareComponentRequestBus::Events::ToggleLensFlare)
                ;

            behaviorContext->EBus<LensFlareComponentNotificationBus>("LensFlareComponentNotificationBus")
                ->Handler<BehaviorLensFlareComponentNotificationBusHandler>();
        }
    }

    void LensFlareConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<LensFlareConfiguration>()->
                Version(4, &VersionConverter)->
                Field("Visible", &LensFlareConfiguration::m_visible)->
                Field("LensFlare", &LensFlareConfiguration::m_lensFlare)->
                Field("Asset", &LensFlareConfiguration::m_asset)->

                Field("MinimumSpec", &LensFlareConfiguration::m_minSpec)->
                Field("LensFlareFrustumAngle", &LensFlareConfiguration::m_lensFlareFrustumAngle)->
                Field("Size", &LensFlareConfiguration::m_size)->
                Field("AttachToSun", &LensFlareConfiguration::m_attachToSun)->
                Field("AffectsThisAreaOnly", &LensFlareConfiguration::m_affectsThisAreaOnly)->
                Field("UseVisAreas", &LensFlareConfiguration::m_useVisAreas)->
                Field("IndoorOnly", &LensFlareConfiguration::m_indoorOnly)->
                Field("OnInitially", &LensFlareConfiguration::m_onInitially)->
                Field("ViewDistanceMultiplier", &LensFlareConfiguration::m_viewDistMultiplier)->

                Field("Tint", &LensFlareConfiguration::m_tint)->
                Field("TintAlpha", &LensFlareConfiguration::m_tintAlpha)->
                Field("Brightness", &LensFlareConfiguration::m_brightness)->

                Field("SyncAnimWithLight", &LensFlareConfiguration::m_syncAnimWithLight)->
                Field("LightEntity", &LensFlareConfiguration::m_lightEntity)->
                Field("AnimIndex", &LensFlareConfiguration::m_animIndex)->
                Field("AnimSpeed", &LensFlareConfiguration::m_animSpeed)->
                Field("AnimPhase", &LensFlareConfiguration::m_animPhase)->

                Field("SyncedAnimIndex", &LensFlareConfiguration::m_syncAnimIndex)->
                Field("SyncedAnimSpeed", &LensFlareConfiguration::m_syncAnimSpeed)->
                Field("SyncedAnimPhase", &LensFlareConfiguration::m_syncAnimPhase)
                ;
        }
    }

    bool LensFlareConfiguration::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 2 to version 3:
        // - Need to rename IgnoreVisAreas to UseVisAreas
        // UseVisAreas is the Inverse of IgnoreVisAreas
        if (classElement.GetVersion() <= 2)
        {
            int ignoreVisAreasIndex = classElement.FindElement(AZ_CRC("IgnoreVisAreas", 0x01823201));

            if (ignoreVisAreasIndex < 0)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& useVisAreasNode = classElement.GetSubElement(ignoreVisAreasIndex);
            useVisAreasNode.SetName("UseVisAreas");

            bool ignoreVisAreas = true;
            if (!useVisAreasNode.GetData<bool>(ignoreVisAreas))
            {
                return false;
            }

            if (!useVisAreasNode.SetData<bool>(context, !ignoreVisAreas))
            {
                return false;
            }
        }

        return true;
    }

    void LensFlareComponent::TurnOnLensFlare()
    {
        bool success = m_light.TurnOn();
        if (success)
        {
            LensFlareComponentNotificationBus::Event(GetEntityId(), &LensFlareComponentNotificationBus::Events::LensFlareTurnedOn);
        }
    }

    void LensFlareComponent::TurnOffLensFlare()
    {
        bool success = m_light.TurnOff();
        if (success)
        {
            LensFlareComponentNotificationBus::Event(GetEntityId(), &LensFlareComponentNotificationBus::Events::LensFlareTurnedOff);
        }
    }

    void LensFlareComponent::ToggleLensFlare()
    {
        if (m_light.IsOn())
        {
            TurnOffLensFlare();
        }
        else
        {
            TurnOnLensFlare();
        }
    }

    LensFlareConfiguration LensFlareComponent::GetLensFlareConfiguration()
    {
        return m_configuration;
    }

    void LensFlareComponent::SetLensFlareState(State state)
    {
        switch (state)
        {
        case LensFlareComponentRequests::State::On:
        {
            TurnOnLensFlare();
        }
        break;

        case LensFlareComponentRequests::State::Off:
        {
            TurnOffLensFlare();
        }
        break;
        }
    }

    IRenderNode* LensFlareComponent::GetRenderNode()
    {
        return m_light.GetRenderNode();
    }

    float LensFlareComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    const float LensFlareComponent::s_renderNodeRequestBusOrder = 600.f;

    void LensFlareComponent::Init() 
    {
        //Set this in Init because it will never change and doesn't need to be reset if the component re-activates
        m_configuration.m_material.SetAssetPath("EngineAssets/Materials/lens_optics");
    }

    void LensFlareComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();

        m_light.SetEntity(entityId);
        m_light.CreateRenderLight(m_configuration);

        LensFlareComponentRequestBus::Handler::BusConnect(entityId);
        RenderNodeRequestBus::Handler::BusConnect(entityId);

        if (m_configuration.m_onInitially)
        {
            TurnOnLensFlare();
        }
        else
        {
            TurnOffLensFlare();
        }
    }

    void LensFlareComponent::Deactivate()
    {
        LensFlareComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();

        m_light.DestroyRenderLight();
        m_light.SetEntity(AZ::EntityId());
    }

    LensFlareConfiguration::LensFlareConfiguration()
        : m_minSpec(EngineSpec::Low)
        , m_visible(true)
        , m_onInitially(true)
        , m_tint(1.f)
        , m_brightness(1.f)
        , m_viewDistMultiplier(1.f)
        , m_affectsThisAreaOnly(1.f)
        , m_useVisAreas(true)
        , m_indoorOnly(false)
        , m_animSpeed(1.f)
        , m_animPhase(0.f)
        , m_animIndex(0)
        , m_lensFlareFrustumAngle(360.f)
        , m_size(1.0f)
        , m_attachToSun(false)
        , m_syncAnimWithLight(false)
    {
    }
} // namespace LmbrCentral
