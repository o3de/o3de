/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Authentication/AuthenticationProviderBus.h>
#include <AzCore/Component/TickBus.h>

namespace AWSClientAuth
{
    //! Authentication behavior EBus handler
    class AuthenticationNotificationBusBehaviorHandler
        : public AuthenticationProviderNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            AuthenticationNotificationBusBehaviorHandler, "{221D74E0-B55A-4086-8B83-B52070A57217}", AZ::SystemAllocator,
                OnPasswordGrantSingleFactorSignInSuccess, OnPasswordGrantSingleFactorSignInFail,
                OnPasswordGrantMultiFactorSignInSuccess, OnPasswordGrantMultiFactorSignInFail,
                OnPasswordGrantMultiFactorConfirmSignInSuccess, OnPasswordGrantMultiFactorConfirmSignInFail,
                OnDeviceCodeGrantSignInSuccess, OnDeviceCodeGrantSignInFail,
                OnDeviceCodeGrantConfirmSignInSuccess, OnDeviceCodeGrantConfirmSignInFail,
                OnRefreshTokensSuccess, OnRefreshTokensFail
            );

        void OnPasswordGrantSingleFactorSignInSuccess(const AuthenticationTokens& authenticationToken) override
        {
            AZ::TickBus::QueueFunction([authenticationToken, this]()
            {
                Call(FN_OnPasswordGrantSingleFactorSignInSuccess, authenticationToken);
            });
        }

        void OnPasswordGrantSingleFactorSignInFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]()
            {
                Call(FN_OnPasswordGrantSingleFactorSignInFail, error);
            });
        }

        void OnPasswordGrantMultiFactorSignInSuccess() override
        {
            AZ::TickBus::QueueFunction([this]()
            {
                Call(FN_OnPasswordGrantMultiFactorSignInSuccess);
            });
        }

        void OnPasswordGrantMultiFactorSignInFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]()
            {
                Call(FN_OnPasswordGrantMultiFactorSignInFail, error);
            });
        }

        void OnPasswordGrantMultiFactorConfirmSignInSuccess(const AuthenticationTokens& authenticationToken) override
        {
            AZ::TickBus::QueueFunction([authenticationToken, this]()
            {
                Call(FN_OnPasswordGrantMultiFactorConfirmSignInSuccess, authenticationToken);
            });
        }

        void OnPasswordGrantMultiFactorConfirmSignInFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]()
            {
                Call(FN_OnPasswordGrantMultiFactorConfirmSignInFail, error);
            });
        }

        void OnDeviceCodeGrantSignInSuccess(
            const AZStd::string& userCode, const AZStd::string& verificationUrl, const int codeExpiresInSeconds) override
        {
            AZ::TickBus::QueueFunction([userCode, verificationUrl, codeExpiresInSeconds, this]()
            {
                Call(FN_OnDeviceCodeGrantSignInSuccess, userCode, verificationUrl, codeExpiresInSeconds);
            });
        }

        void OnDeviceCodeGrantSignInFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]()
            {
                Call(FN_OnDeviceCodeGrantSignInFail, error);
            });
        }

        void OnDeviceCodeGrantConfirmSignInSuccess(const AuthenticationTokens& authenticationToken) override
        {
            AZ::TickBus::QueueFunction([authenticationToken, this]()
            {
                Call(FN_OnDeviceCodeGrantConfirmSignInSuccess, authenticationToken);
            });
        }

        void OnDeviceCodeGrantConfirmSignInFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]()
            {
                Call(FN_OnDeviceCodeGrantConfirmSignInFail, error);
            });
        }

        void OnRefreshTokensSuccess(const AuthenticationTokens& authenticationToken) override
        {
            AZ::TickBus::QueueFunction([authenticationToken, this]()
            {
                Call(FN_OnRefreshTokensSuccess, authenticationToken);
            });
        }

        void OnRefreshTokensFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]()
            {
                Call(FN_OnRefreshTokensFail, error);
            });
        }
    };
} // namespace AWSClientAuth
