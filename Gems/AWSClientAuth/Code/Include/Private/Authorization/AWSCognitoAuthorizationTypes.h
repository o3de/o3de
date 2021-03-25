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

#include <AzCore/Serialization/EditContext.h>

namespace AWSClientAuth
{
    //! Holds Cognito Authorization Identity pool settings.
    class CognitoAuthorizationSettings
    {
    public:

        AZ_TYPE_INFO(CognitoAuthorizationSettings, "{2F2080CD-E575-42BD-9717-E42E43C13956}");

        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<CognitoAuthorizationSettings>()
                ->Field("CognitoUserPoolId", &CognitoAuthorizationSettings::m_cognitoUserPoolId)
                ->Field("LoginWithAmazonId", &CognitoAuthorizationSettings::m_loginWithAmazonId)
                ->Field("GoogleId", &CognitoAuthorizationSettings::m_googleId)
                ->Field("AWSAccountId", &CognitoAuthorizationSettings::m_awsAccountId)
                ->Field("IdentityPoolId", &CognitoAuthorizationSettings::m_cognitoIdentityPoolId);

            AZ::EditContext* editContext = context.GetEditContext();

            if (editContext)
            {
                editContext->Class<CognitoAuthorizationSettings>("CognitoAuthorizationSettings", "CognitoAuthorizationSettings")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "AWSClientAuth")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CognitoAuthorizationSettings::m_cognitoUserPoolId, "CognitoUserPoolId", "Cognito User pool Id")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CognitoAuthorizationSettings::m_loginWithAmazonId, "LoginWithAmazonId", "Login with Amazon id. default: www.amazon.com")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CognitoAuthorizationSettings::m_googleId, "Google Endpoint", "Google endpoint. default: accounts.google.com")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CognitoAuthorizationSettings::m_cognitoUserPoolId, "AWSAccountId", "AWS account Cognito for Cognito identity pool")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CognitoAuthorizationSettings::m_cognitoUserPoolId, "IdentityPoolId", "Cognito Identity pool Id")
                    ;
            }
         
        }

        AZStd::string m_cognitoUserPoolId;
        AZStd::string m_loginWithAmazonId = "www.amazon.com";
        AZStd::string m_googleId = "accounts.google.com";
        AZStd::string m_awsAccountId;
        AZStd::string m_cognitoIdentityPoolId;
    };
} // namespace AWSClientAuth
