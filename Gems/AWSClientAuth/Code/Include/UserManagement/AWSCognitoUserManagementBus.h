/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>

namespace AWSClientAuth
{
    //! Abstract class for AWS Cognito user management requests.
    class IAWSCognitoUserManagementRequests
    {
    public:
        AZ_TYPE_INFO(IAWSCognitoUserManagementRequests, "{A4C90F21-7056-4827-8C6B-401E6945697D}");

        //! Initialize Cognito User pool using settings from resource mappings.
        //! @param settingsRegistryPath settingsRegistryPath Path for the settings registry file to use.
        virtual bool Initialize() = 0;

        // Requests interface

        //! Cognito user pool email sign up start.
        //! @param username User name to use for sign up.
        //! @param password Password to use for sign up.
        //! @param email Email used to send confirmation code.
        virtual void EmailSignUpAsync(const AZStd::string& userName, const AZStd::string& password, const AZStd::string& email) = 0;

        //! Cognito user pool phone sign up start.
        //! @param username User name to use for sign up.
        //! @param password Password to use for sign up.
        //! @param phoneNumber Phone number used to send confirmation code.
        virtual void PhoneSignUpAsync(const AZStd::string& userName, const AZStd::string& password, const AZStd::string& phoneNumber) = 0;

        //! Cognito user pool confirm sign up with confirmation code. Used to confirm email or phone sign up.
        //! @param username User name to use to confirm sign up.
        //! @param confirmationCode Code sent to email/phone from sign up call.
        virtual void ConfirmSignUpAsync(const AZStd::string& userName, const AZStd::string& confirmationCode) = 0;

        //! Cognito user forgot password start
        //! @param username User name to use to reset password for.
        virtual void ForgotPasswordAsync(const AZStd::string& userName) = 0;

        //! Cognito user pool confirm forgot password with confirmation code.
        //! @param username User name to use to confirm reset password for.
        //! @param confirmationCode Code sent to email/phone for forgot password step.
        //! @param newPassword New password to set the changed value to.
        virtual void ConfirmForgotPasswordAsync(const AZStd::string& userName, const AZStd::string& confirmationCode, const AZStd::string& newPassword) = 0;

        //! Cognito user pool enable multi factor authentication for signed in user.
        //! @param accessToken Access token from successful sign in.
        virtual void EnableMFAAsync(const AZStd::string& accessToken) = 0;
    };

    //! Implements AWS Cognito user pool user management requests.
    class AWSCognitoUserManagementRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        using MutexType = AZ::NullMutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };
    using AWSCognitoUserManagementRequestBus = AZ::EBus<IAWSCognitoUserManagementRequests, AWSCognitoUserManagementRequests>;


    class AWSCognitoUserManagementNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////ss//////////////////////////////////////////////
        // Notifications interface
        //! Event for Cognito user pool email sign up success
        //! @param uuid Unique identified from Cognito User pool for new user
        virtual void OnEmailSignUpSuccess(const AZStd::string& uuid)
        {
            AZ_UNUSED(uuid);
        }

        //! Event for Cognito user pool email sign up fail
        //! @param error Error message
        virtual void OnEmailSignUpFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for Cognito user pool phone sign up success
        //! @param error Error message
        virtual void OnPhoneSignUpSuccess(const AZStd::string& uuid)
        {
            AZ_UNUSED(uuid);
        }

        //! Event for Cognito user pool phone sign up fail
        //! @param error Error message
        virtual void OnPhoneSignUpFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for Cognito confirm sign up success
        virtual void OnConfirmSignUpSuccess()
        {
        }

        //! Event for Cognito confirm sign up fail
        //! @param error Error message
        virtual void OnConfirmSignUpFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for Cognito forgot password success
        virtual void OnForgotPasswordSuccess()
        {
        }

        //! Event for Cognito forgot password fail
        //! @param error Error message
        virtual void OnForgotPasswordFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for Cognito confirm forgot password success
        virtual void OnConfirmForgotPasswordSuccess()
        {
        }

        //! Event for Cognito confirm forgot password fail
        //! @param error Error message
        virtual void OnConfirmForgotPasswordFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }

        //! Event for Cognito enable mfa success
        virtual void OnEnableMFASuccess()
        {
        }

        //! Event for Cognito enable mfa fail
        //! @param error Error message
        virtual void OnEnableMFAFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }
        //////////////////////////////////////////////////////////////////////////
    };
    using AWSCognitoUserManagementNotificationBus = AZ::EBus<AWSCognitoUserManagementNotifications>;
} // namespace AWSClientAuth
