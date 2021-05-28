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
#pragma once

#include <UserManagement/AWSCognitoUserManagementBus.h>
#include <AzCore/Component/TickBus.h>

namespace AWSClientAuth
{
    //! User management behavior EBus handler
    class UserManagementNotificationBusBehaviorHandler
        : public AWSCognitoUserManagementNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(UserManagementNotificationBusBehaviorHandler, "{57289595-2CDC-4834-8017-4A96B983E028}", AZ::SystemAllocator,
            OnEmailSignUpSuccess, OnEmailSignUpFail,
            OnPhoneSignUpSuccess, OnPhoneSignUpFail,
            OnConfirmSignUpSuccess, OnConfirmSignUpFail,
            OnForgotPasswordSuccess, OnForgotPasswordFail,
            OnConfirmForgotPasswordSuccess, OnConfirmForgotPasswordFail,
            OnEnableMFASuccess, OnEnableMFAFail
        );

        void OnEmailSignUpSuccess(const AZStd::string& uuid) override
        {
            AZ::TickBus::QueueFunction([uuid, this]() {
                Call(FN_OnEmailSignUpSuccess, uuid);
            });
        }

        void OnEmailSignUpFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]() {
                Call(FN_OnEmailSignUpFail, error);
            });
        }

        void OnPhoneSignUpSuccess(const AZStd::string& uuid) override
        {
            AZ::TickBus::QueueFunction([uuid, this]() {
                Call(FN_OnPhoneSignUpSuccess, uuid);
            });
        }

        void OnPhoneSignUpFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]() {
                Call(FN_OnPhoneSignUpFail, error);
            });
        }

        void OnConfirmSignUpSuccess() override
        {
            AZ::TickBus::QueueFunction([this]() {
                Call(FN_OnConfirmSignUpSuccess);
            });
        }

        void OnConfirmSignUpFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]() {
                Call(FN_OnConfirmSignUpFail, error);
            });
        }

        void OnForgotPasswordSuccess() override
        {
            AZ::TickBus::QueueFunction([this]() {
                Call(FN_OnForgotPasswordSuccess);
            });
        }

        void OnForgotPasswordFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]() {
                Call(FN_OnForgotPasswordFail, error);
            });
        }

        void OnConfirmForgotPasswordSuccess() override
        {
            AZ::TickBus::QueueFunction([this]() {
                Call(FN_OnConfirmForgotPasswordSuccess);
            });
        }

        void OnConfirmForgotPasswordFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]() {
                Call(FN_OnConfirmForgotPasswordFail, error);
            });
        }

        void OnEnableMFASuccess() override
        {
            AZ::TickBus::QueueFunction([this]() {
                Call(FN_OnEnableMFASuccess);
            });
        }

        void OnEnableMFAFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]() {
                Call(FN_OnEnableMFAFail, error);
            });
        }
    };
} // namespace AWSClientAuth
