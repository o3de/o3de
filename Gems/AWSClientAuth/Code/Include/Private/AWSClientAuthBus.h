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

#include <AzCore/EBus/EBus.h>

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
