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

#include <Authorization/AWSCognitoAuthorizationBus.h>
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
            Call(FN_OnRequestAWSCredentialsSuccess, awsCredentials);
        }

        void OnRequestAWSCredentialsFail(const AZStd::string& error) override
        {
            Call(FN_OnRequestAWSCredentialsFail, error);
        }
    };
} // namespace AWSClientAuth
