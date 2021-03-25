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

#include <Authentication/AuthenticationProviderBus.h>

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
                OnRefreshTokensSuccess, OnRefreshTokensFail,
                OnSignOut
            );

        void OnPasswordGrantSingleFactorSignInSuccess(const AuthenticationTokens& authenticationToken) override
        {
            Call(FN_OnPasswordGrantSingleFactorSignInSuccess, authenticationToken);
        }

        void OnPasswordGrantSingleFactorSignInFail(const AZStd::string& error) override
        {
            Call(FN_OnPasswordGrantSingleFactorSignInFail, error);
        }

        void OnPasswordGrantMultiFactorSignInSuccess() override
        {
            Call(FN_OnPasswordGrantMultiFactorSignInSuccess);
        }

        void OnPasswordGrantMultiFactorSignInFail(const AZStd::string& error) override
        {
            Call(FN_OnPasswordGrantMultiFactorSignInFail, error);
        }

        void OnPasswordGrantMultiFactorConfirmSignInSuccess(const AuthenticationTokens& authenticationToken) override
        {
            Call(FN_OnPasswordGrantMultiFactorConfirmSignInSuccess, authenticationToken);
        }

        void OnPasswordGrantMultiFactorConfirmSignInFail(const AZStd::string& error) override
        {
            Call(FN_OnPasswordGrantMultiFactorConfirmSignInFail, error);
        }

        void OnDeviceCodeGrantSignInSuccess(
            const AZStd::string& userCode, const AZStd::string& verificationUrl, const int codeExpiresInSeconds) override
        {
            Call(FN_OnDeviceCodeGrantSignInSuccess, userCode, verificationUrl, codeExpiresInSeconds);
        }

        void OnDeviceCodeGrantSignInFail(const AZStd::string& error) override
        {
            Call(FN_OnDeviceCodeGrantSignInFail, error);
        }

        void OnDeviceCodeGrantConfirmSignInSuccess(const AuthenticationTokens& authenticationToken) override
        {
            Call(FN_OnDeviceCodeGrantConfirmSignInSuccess, authenticationToken);
        }

        void OnDeviceCodeGrantConfirmSignInFail(const AZStd::string& error) override
        {
            Call(FN_OnDeviceCodeGrantConfirmSignInFail, error);
        }

        void OnRefreshTokensSuccess(const AuthenticationTokens& authenticationToken) override
        {
            Call(FN_OnRefreshTokensSuccess, authenticationToken);
        }

        void OnRefreshTokensFail(const AZStd::string& error) override
        {
            Call(FN_OnRefreshTokensFail, error);
        }

        void OnSignOut(const ProviderNameEnum& provideName) override
        {
            Call(FN_OnSignOut, provideName);
        }
    };
} // namespace AWSClientAuth
