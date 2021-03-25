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

#include <UserManagement/AWSCognitoUserManagementBus.h>
#include <UserManagement/AWSCognitoUserManagementTypes.h>

namespace AWSClientAuth
{
    //! Implements AWS Cognito User pool user management.
    class AWSCognitoUserManagementController
        : public AWSCognitoUserManagementRequestBus::Handler
    {
    public:
        AZ_RTTI(AWSCognitoUserManagementController, "{2645D1CC-EB55-4A8D-8F45-5DFE94032813}", IAWSCognitoUserManagementRequests);
        AWSCognitoUserManagementController();
        virtual ~AWSCognitoUserManagementController();

        // AWSCognitoUserManagementRequestsBus interface methods
        bool Initialize(const AZStd::string& settingsRegistryPath);
        void EmailSignUpAsync(const AZStd::string& username, const AZStd::string& password, const AZStd::string& email) override;
        void PhoneSignUpAsync(const AZStd::string& username, const AZStd::string& password, const AZStd::string& phoneNumber) override;
        void ConfirmSignUpAsync(const AZStd::string& username, const AZStd::string& confirmationCode) override;
        void ForgotPasswordAsync(const AZStd::string& username) override;
        void ConfirmForgotPasswordAsync(const AZStd::string& userName, const AZStd::string& confirmationCode, const AZStd::string& newPassword) override;
        void EnableMFAAsync(const AZStd::string& accessToken) override;

    protected:
        AZStd::unique_ptr<AWSCognitoUserManagementSetting> m_settings;
    };

} // namespace AWSClientAuth
