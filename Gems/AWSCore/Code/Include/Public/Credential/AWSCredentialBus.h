/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace Aws
{
    namespace Auth
    {
        class AWSCredentialsProvider;
    }
} // namespace Aws

namespace AWSCore
{
    enum CredentialHandlerOrder
    {
        CVAR_CREDENTIAL_HANDLER = 0,
        COGNITO_IDENITY_POOL_CREDENTIAL_HANDLER = 20,
        DEFAULT_CREDENTIAL_HANDLER = 30,
    };

    //! Aggregates credentials provider results returned by all handlers
    //! which takes the first valid credential provider following the order
    struct AWSCredentialResult
    {
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> result;

        AWSCredentialResult() : result(nullptr){}

        //! Overloads the assignment operator to aggregate a new value with
        //! the existing aggregated value using rvalue
        void operator=(std::shared_ptr<Aws::Auth::AWSCredentialsProvider>&& rhs)
        {
            if (!result)
            {
                result = AZStd::move(rhs);
            }
        }
    };

    //! AWSCredential request interface
    class AWSCredentialRequests : public AZ::EBusTraits
    {
    public:
        // Allow multiple threads to concurrently make requests
        using MutexType = AZStd::recursive_mutex;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Determines the order in which handlers get credentials provider.
        struct BusHandlerOrderCompare
        {
            AZ_FORCE_INLINE bool operator()(AWSCredentialRequests* left, AWSCredentialRequests* right) const
            {
                return left->GetCredentialHandlerOrder() < right->GetCredentialHandlerOrder();
            }
        };

        //! GetCredentialHandlerOrder
        //! Get the order of credential handler
        //! @return The value of credential handler order
        virtual int GetCredentialHandlerOrder() const = 0;

        //! GetCredentialsProvider
        //! Get credential provider to supply required AWS credential for making requests
        //! to Amazon Web Services
        //! @return The credential provider
        virtual std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() = 0;
    };

    using AWSCredentialRequestBus = AZ::EBus<AWSCredentialRequests>;

} // namespace AWSCore
