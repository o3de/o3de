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
#include <Authorization/ClientAuthAWSCredentials.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

namespace AWSClientAuth
{
    //! Abstract class for AWS Cognito authorization requests.
    class IAWSCognitoAuthorizationRequests
    {
    public:
        AZ_TYPE_INFO(IAWSCognitoAuthorizationRequests, "{F60A2C40-48F5-49A1-ABFA-A08D0DD4ECCC}");

        //! Initializes settings for Cognito identity pool from settings registry.
        //! @param settingsRegistryPath Path for the settings registry file to use.
        virtual bool Initialize() = 0;

        //! Once credentials provider are set they cannot be reset. So recreates new Cognito credentials provider on reset.
        //! Service clients need to be created with the new AWSCredentialsProvider after reset.
        virtual void Reset() = 0;

        //! Get cached Cognito identity id from last successful GetId call to Cognito.
        //! @return Cognito identity id
        virtual AZStd::string GetIdentityId() = 0;

        //! Checks if logins are persisted.
        //! @return True if logins persists else false.
        virtual bool HasPersistedLogins() = 0;

        //! Returns AWSCredentialsProvider to initialize up AWS Native SDK clients.
        //! std::shared_ptr to allow sharing ownership with AWS Native SDK.
        //! @return std::shared_ptr for Aws::Auth::AWSCredentialProvider.
        virtual std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCognitoCredentialsProvider() = 0;

        //! Returns anonymous AWSCredentialsProvider to initialize up AWS Native SDK clients.
        //! std::shared_ptr to allow sharing ownership with AWS Native SDK.
        //! @return std::shared_ptr for Aws::Auth::AWSCredentialProvider.
        virtual std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetAnonymousCognitoCredentialsProvider() = 0;

        //! Get cached AWS credentials or fetch credentials from Cognito.
        //! Will fetch authenticated role credentials if login are cached else fetches unauthenticated role credentials if enabled in Cognito Identity pool.
        //! If multiple logins are persisted and no cached credentials found, GetId call to Cognito will link the login provider to same identity.
        virtual void RequestAWSCredentialsAsync() = 0;
    };

    //! Request bus to handle AWS Cognito Identity pool authorization.
    class AWSCognitoAuthorizationRequests
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
    using AWSCognitoAuthorizationRequestBus = AZ::EBus<IAWSCognitoAuthorizationRequests, AWSCognitoAuthorizationRequests>;

    //! Notification bus for corresponding Authorization Request bus.
    class AWSCognitoAuthorizationNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Notifications interface

        //! Event called on request AWS credentials success.
        //! @param awsCredentials Credentials for authenticated role associated with Cognito identity pool.
        virtual void OnRequestAWSCredentialsSuccess(const ClientAuthAWSCredentials& awsCredentials)
        {
            AZ_UNUSED(awsCredentials);
        }

        //! Event called on request AWS credentials fail.
        //! @param error Error message
        virtual void OnRequestAWSCredentialsFail(const AZStd::string& error)
        {
            AZ_UNUSED(error);
        }
        //////////////////////////////////////////////////////////////////////////

    };
    using AWSCognitoAuthorizationNotificationBus = AZ::EBus<AWSCognitoAuthorizationNotifications>;
} // namespace AWSClientAuth
