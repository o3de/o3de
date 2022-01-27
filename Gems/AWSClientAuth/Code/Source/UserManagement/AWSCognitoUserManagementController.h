/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <UserManagement/AWSCognitoUserManagementBus.h>

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
        bool Initialize() override;
        void EmailSignUpAsync(const AZStd::string& username, const AZStd::string& password, const AZStd::string& email) override;
        void PhoneSignUpAsync(const AZStd::string& username, const AZStd::string& password, const AZStd::string& phoneNumber) override;
        void ConfirmSignUpAsync(const AZStd::string& username, const AZStd::string& confirmationCode) override;
        void ForgotPasswordAsync(const AZStd::string& username) override;
        void ConfirmForgotPasswordAsync(const AZStd::string& userName, const AZStd::string& confirmationCode, const AZStd::string& newPassword) override;
        void EnableMFAAsync(const AZStd::string& accessToken) override;

        inline const AZStd::string& GetCognitoAppClientId() const
        {
            return m_cognitoAppClientId;
        }

    private:
        AZStd::string m_cognitoAppClientId;
    };

} // namespace AWSClientAuth
