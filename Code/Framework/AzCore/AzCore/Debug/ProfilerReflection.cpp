/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/ProfilerBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/BehaviorInterfaceProxy.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace AZ::Debug
{
    static constexpr const char* ProfilerScriptCategory = "Profiler";
    static constexpr const char* ProfilerScriptModule = "debug";
    static constexpr AZ::Script::Attributes::ScopeFlags ProfilerScriptScope = AZ::Script::Attributes::ScopeFlags::Automation;

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
                    ->Attribute(AZ::Script::Attributes::Category, ProfilerScriptCategory)
                    ->Attribute(AZ::Script::Attributes::Module, ProfilerScriptModule)
                    ->Attribute(AZ::Script::Attributes::Scope, ProfilerScriptScope)
                    ->Handler<ProfilerNotificationBusHandler>();
            }
        }
    };

    class ProfilerSystemScriptProxy
        : public BehaviorInterfaceProxy<ProfilerRequests>
    {
    public:
        AZ_RTTI(ProfilerSystemScriptProxy, "{D671FB70-8B09-4C3A-96CD-06A339F3138E}", BehaviorInterfaceProxy<ProfilerRequests>);

        AZ_BEHAVIOR_INTERFACE(ProfilerSystemScriptProxy, ProfilerRequests);
    };

    void ProfilerReflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("g_ProfilerSystem", ProfilerSystemScriptProxy::GetProxy)
                ->Attribute(AZ::Script::Attributes::Category, ProfilerScriptCategory)
                ->Attribute(AZ::Script::Attributes::Module, ProfilerScriptModule)
                ->Attribute(AZ::Script::Attributes::Scope, ProfilerScriptScope);

            behaviorContext->Class<ProfilerSystemScriptProxy>("ProfilerSystemInterface")
                ->Attribute(AZ::Script::Attributes::Category, ProfilerScriptCategory)
                ->Attribute(AZ::Script::Attributes::Module, ProfilerScriptModule)
                ->Attribute(AZ::Script::Attributes::Scope, ProfilerScriptScope)

                ->Method("IsValid", &ProfilerSystemScriptProxy::IsValid)

                ->Method("GetCaptureLocation",
                    [](ProfilerSystemScriptProxy*) -> AZStd::string
                    {
                        AZ::IO::FixedMaxPathString captureOutput = GetProfilerCaptureLocation();
                        return AZStd::string(captureOutput.c_str(), captureOutput.length());
                    })

                ->Method("IsActive", ProfilerSystemScriptProxy::WrapMethod<&ProfilerRequests::IsActive>())
                ->Method("SetActive", ProfilerSystemScriptProxy::WrapMethod<&ProfilerRequests::SetActive>())

                ->Method("CaptureFrame", ProfilerSystemScriptProxy::WrapMethod<&ProfilerRequests::CaptureFrame>())

                ->Method("StartCapture", ProfilerSystemScriptProxy::WrapMethod<&ProfilerRequests::StartCapture>())
                ->Method("EndCapture", ProfilerSystemScriptProxy::WrapMethod<&ProfilerRequests::EndCapture>());
        }

        ProfilerNotificationBusHandler::Reflect(context);
    }
} // namespace AZ::Debug
