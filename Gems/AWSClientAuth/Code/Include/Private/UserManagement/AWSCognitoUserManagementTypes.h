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

    //! Holds AWS Cognito user management serialized Settings.
    class AWSCognitoUserManagementSetting
    {
    public:
        AWSCognitoUserManagementSetting() = default;
        ~AWSCognitoUserManagementSetting() = default;

        AZ_TYPE_INFO(AWSCognitoUserManagementSetting, "{58FC34F1-B84B-4677-B986-45A226F0328D}");

        AZStd::string m_appClientId;

        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<AWSCognitoUserManagementSetting>()
                ->Field("AppClientId", &AWSCognitoUserManagementSetting::m_appClientId)
                ;

            AZ::EditContext* editContext = context.GetEditContext();

            if (editContext)
            {
                editContext->Class<AWSCognitoUserManagementSetting>("AWSCognitoUserManagementSetting", "AWSCognitoUserManagementSetting Settings")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "AWSClientAuth")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AWSCognitoUserManagementSetting::m_appClientId, "ClientId", "Cognito User Pool App Client Id")
                    ;
            }
        }
    };
} // namespace AWSClientAuth
