/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/ProfilerBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace AZ::Debug
{
    class ProfilerNotificationBusHandler final
        : public ProfilerNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ProfilerNotificationBusHandler, "{44161459-B816-4876-95A4-BA16DEC767D6}", AZ::SystemAllocator,
            OnCaptureFinished
        );

        void OnCaptureFinished(bool result, const AZStd::string& info) override
        {
            Call(FN_OnCaptureFinished, result, info);
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<ProfilerNotificationBus>("ProfilerNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "debug")
                    ->Handler<ProfilerNotificationBusHandler>();
            }
        }
    };

    void ProfilerReflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ProfilerNotificationBusHandler::Reflect(context);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ProfilerRequestBus>("ProfilerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "debug")

                ->Event("IsActive", &ProfilerRequestBus::Events::IsActive)
                ->Event("SetActive", &ProfilerRequestBus::Events::SetActive)

                ->Event("CaptureFrame", &ProfilerRequestBus::Events::CaptureFrame)

                ->Event("StartCapture", &ProfilerRequestBus::Events::StartCapture)
                ->Event("EndCapture", &ProfilerRequestBus::Events::EndCapture);

            ProfilerNotificationBusHandler::Reflect(context);
        }
    }
} // namespace AZ::Debug
