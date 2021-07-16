/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Authorization/AWSCognitoAuthorizationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AWSClientAuth
{
    //! Authorization behavior EBus handler
    class AWSCognitoAuthorizationNotificationBusBehaviorHandler
        : public AWSCognitoAuthorizationNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(AWSCognitoAuthorizationNotificationBusBehaviorHandler, "{F2BCAB42-97FD-41AC-AF7A-7E3BD64B7089}", AZ::SystemAllocator,
            OnRequestAWSCredentialsSuccess, OnRequestAWSCredentialsFail
        );

        void OnRequestAWSCredentialsSuccess(const ClientAuthAWSCredentials& awsCredentials) override
        {
            AZ::TickBus::QueueFunction([awsCredentials, this]()
            {
                Call(FN_OnRequestAWSCredentialsSuccess, awsCredentials);
            });
        }

        void OnRequestAWSCredentialsFail(const AZStd::string& error) override
        {
            AZ::TickBus::QueueFunction([error, this]()
            {
                Call(FN_OnRequestAWSCredentialsFail, error);
            });
        }
    };
} // namespace AWSClientAuth
