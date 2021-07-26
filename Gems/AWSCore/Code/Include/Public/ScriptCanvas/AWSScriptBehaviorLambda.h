/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AWSCore
{
    //! AWS Script Behavior notifications for ScriptCanvas behaviors that interact with AWS Lambda
    class AWSScriptBehaviorLambdaNotifications
        : public AZ::EBusTraits
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Called when a successful script behavior lambda invoke call has occurred
        virtual void OnInvokeSuccess(const AZStd::string& resultBody) = 0;

        //! Called when script behavior lambda invoke call has failed
        virtual void OnInvokeError(const AZStd::string& errorBody) = 0;

    };
    using AWSScriptBehaviorLambdaNotificationBus = AZ::EBus<AWSScriptBehaviorLambdaNotifications>;

    class AWSScriptBehaviorLambdaNotificationBusHandler
        : public AWSScriptBehaviorLambdaNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(AWSScriptBehaviorLambdaNotificationBusHandler, "{533E8AC9-CBD7-4718-9FBB-622C5E70045F}",
            AZ::SystemAllocator, OnInvokeSuccess, OnInvokeError);

        void OnInvokeSuccess(const AZStd::string& resultBody) override
        {
            Call(FN_OnInvokeSuccess, resultBody);
        }

        void OnInvokeError(const AZStd::string& errorBody) override
        {
            Call(FN_OnInvokeError, errorBody);
        }
    };

    class AWSScriptBehaviorLambda
    {
    public:
        AZ_RTTI(AWSScriptBehaviorLambda, "{9E71534D-34B3-4723-B180-2552513DDA3D}");

        AWSScriptBehaviorLambda() = default;
        virtual ~AWSScriptBehaviorLambda() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void Invoke(const AZStd::string& functionResourceKey, const AZStd::string& payload);
        static void InvokeRaw(const AZStd::string& functionName, const AZStd::string& payload, const AZStd::string& region);

    private:
        static bool ValidateInvokeRequest(const AZStd::string& functionName, const AZStd::string& region);
    };
} // namespace AWSCore
