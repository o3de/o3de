/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

#include <memory>

namespace Aws
{
    namespace CognitoIdentityProvider
    {
        class CognitoIdentityProviderClient;
    }

    namespace CognitoIdentity
    {
        class CognitoIdentityClient;
    }
}

namespace AWSClientAuth
{
    //! Abstract class for AWS client auth requests.
    class IAWSClientAuthRequests
    {
    public:
        AZ_TYPE_INFO(IAWSClientAuthRequests, "{1798CB8B-A334-40BD-913A-4739BF939201}");

        //! std shared_ptr as the ownership has to be shared with AWS Native SDK.
        //! @return AWS Native SDK Cognito IDP client
        virtual std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> GetCognitoIDPClient() = 0;

        //! std shared_ptr as the ownership has to be shared with AWS Native SDK.
        //! @return AWS Native SDK Cognito Identity client
        virtual std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient> GetCognitoIdentityClient() = 0;
    };

    //! Responsible for fetching AWS Cognito IDP and Identity service client objetcs.
    class AWSClientAuthRequests
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
    using AWSClientAuthRequestBus = AZ::EBus<IAWSClientAuthRequests, AWSClientAuthRequests>;

} // namespace AWSClientAuth
