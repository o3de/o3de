/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PresenceSystemComponent.h>
#include <Presence/PresenceNotificationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Presence
{
    class PresenceNotificationBusBehaviorHandler
        : public PresenceNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_EBUS_BEHAVIOR_BINDER(PresenceNotificationBusBehaviorHandler, "{6ECFBA30-CBAA-498F-BE71-01C78B0215EA}", AZ::SystemAllocator
            , OnPresenceSet
            , OnPresenceQueried
        );

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnPresenceSet(const AzFramework::LocalUserId& localUserId) override
        {
            Call(FN_OnPresenceSet, localUserId);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnPresenceQueried(const PresenceDetails& details) override
        {
            Call(FN_OnPresenceQueried, details);
        }

    };

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceDetails::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PresenceDetails>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<PresenceDetails>("PresenceDetails", "Struct to hold platform agnostic presence details for query results")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PresenceDetails>()
                ->Constructor<const PresenceDetails&>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("localUserId", BehaviorValueProperty(&PresenceDetails::localUserId))
                ->Property("titleId", BehaviorValueProperty(&PresenceDetails::titleId))
                ->Property("title", BehaviorValueProperty(&PresenceDetails::title))
                ->Property("presence", BehaviorValueProperty(&PresenceDetails::presence));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    PresenceDetails::PresenceDetails() :
        localUserId(0),
        titleId(0),
        title(""),
        presence("")
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PresenceSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<PresenceSystemComponent>("Presence", "Platform agnostic interface for Presence API requests")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            using SetPresenceParams = PresenceRequests::SetPresenceParams;
            behaviorContext->Class<SetPresenceParams>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("localUserId", BehaviorValueProperty(&SetPresenceParams::localUserId))
                ->Property("presenceToken", BehaviorValueProperty(&SetPresenceParams::presenceToken))
                ->Property("presenceString", BehaviorValueProperty(&SetPresenceParams::presenceString))
                ->Property("languageCode", BehaviorValueProperty(&SetPresenceParams::languageCode));

            using QueryPresenceParams = PresenceRequests::QueryPresenceParams;
            behaviorContext->Class<QueryPresenceParams>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("presence", BehaviorValueProperty(&QueryPresenceParams::presence))
                ->Property("localUserId", BehaviorValueProperty(&QueryPresenceParams::localUserId));

            behaviorContext->EBus<PresenceNotificationBus>("PresenceNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Handler<PresenceNotificationBusBehaviorHandler>();

            behaviorContext->EBus<PresenceRequestBus>("PresenceRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Presence")
                ->Event("SetPresence", &PresenceRequestBus::Events::SetPresence)
                ->Event("QueryPresence", &PresenceRequestBus::Events::QueryPresence);
        }

        PresenceDetails::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PresenceService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PresenceService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::Activate()
    {
        m_pimpl.reset(Implementation::Create(*this));
        PresenceRequestBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::Deactivate()
    {
        PresenceRequestBus::Handler::BusDisconnect();
        m_pimpl.reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::SetPresence(const SetPresenceParams& params)
    {
    #if defined(DEBUG_PRESENCE)
        AZ_Printf("Presence", "Setting presence for localuserId %u", params.localUserId);
    #endif
        if (m_pimpl)
        {
            m_pimpl->SetPresence(params);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::QueryPresence(const QueryPresenceParams& params)
    {
    #if defined(DEBUG_PRESENCE)
        AZ_Printf("Presence", "Querying presence info for localuserId %u", params.localUserId);
    #endif
        if (m_pimpl)
        {
            m_pimpl->QueryPresence(params);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    PresenceSystemComponent::Implementation::Implementation(PresenceSystemComponent& presenceSystemComponent)
        : m_presenceSystemComponent(presenceSystemComponent)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    PresenceSystemComponent::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::Implementation::OnPresenceSetComplete(const SetPresenceParams& params)
    {
        AZ::TickBus::QueueFunction([params]()
        {
            if (params.OnPresenceSetCallback)
            {
                params.OnPresenceSetCallback(params.localUserId);
            }
            PresenceNotificationBus::Broadcast(&PresenceNotifications::OnPresenceSet, params.localUserId);
        });
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void PresenceSystemComponent::Implementation::OnPresenceQueriedComplete(const QueryPresenceParams& params, const PresenceDetails& details)
    {
        AZ::TickBus::QueueFunction([params, details ]()
        {
            if (params.OnQueryPresenceCallback)
            {
                params.OnQueryPresenceCallback(details);
            }
            PresenceNotificationBus::Broadcast(&PresenceNotifications::OnPresenceQueried, details);
        });
    }

}
