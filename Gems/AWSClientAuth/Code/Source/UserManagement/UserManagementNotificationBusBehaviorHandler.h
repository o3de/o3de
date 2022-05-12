/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
