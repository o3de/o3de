/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobFunction.h>

#include <UserManagement/AWSCognitoUserManagementController.h>
#include <AWSClientAuthBus.h>
#include <AWSClientAuthResourceMappingConstants.h>
#include <AWSCoreBus.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/cognito-idp/model/SignUpRequest.h>
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/cognito-idp/model/SignUpResult.h>
#include <aws/cognito-idp/model/ConfirmSignUpRequest.h>
#include <aws/cognito-idp/model/ConfirmSignUpResult.h>
#include <aws/cognito-idp/model/ConfirmSignUpRequest.h>
#include <aws/cognito-idp/model/ConfirmSignUpResult.h>
#include <aws/cognito-idp/model/AttributeType.h>
#include <aws/cognito-idp/model/ForgotPasswordRequest.h>
#include <aws/cognito-idp/model/ForgotPasswordResult.h>
#include <aws/cognito-idp/model/ConfirmForgotPasswordRequest.h>
#include <aws/cognito-idp/model/ConfirmForgotPasswordResult.h>
#include <aws/cognito-idp/model/SetUserMFAPreferenceRequest.h>
#include <aws/cognito-idp/model/SetUserMFAPreferenceResult.h>

namespace AWSClientAuth
{
    AWSCognitoUserManagementController::AWSCognitoUserManagementController()
    {
        AZ::Interface<IAWSCognitoUserManagementRequests>::Register(this);
        AWSCognitoUserManagementRequestBus::Handler::BusConnect();
    }

    AWSCognitoUserManagementController::~AWSCognitoUserManagementController()
    {
        AWSCognitoUserManagementRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAWSCognitoUserManagementRequests>::Unregister(this);
    }

    bool AWSCognitoUserManagementController::Initialize()
    {
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
            m_cognitoAppClientId, &AWSCore::AWSResourceMappingRequests::GetResourceNameId, CognitoAppClientIdResourceMappingKey);
        AZ_Warning(
            "AWSCognitoUserManagementController", !m_cognitoAppClientId.empty(), "Missing Cognito App Client Id from resource mappings. Calls to Cognito will fail.");
        return !m_cognitoAppClientId.empty();
    }

    // Call Cognito user pool sign up using email. Confirmation code sent to the email set.
    // Refer https://docs.aws.amazon.com/cognito/latest/developerguide/signing-up-users-in-your-app.html
    void AWSCognitoUserManagementController::EmailSignUpAsync(const AZStd::string& username, const AZStd::string& password, const AZStd::string& email)
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();
        
        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);

        AZ::Job* emailSignUpJob = AZ::CreateJobFunction([this, cognitoIdentityProviderClient, username, password, email]()
        {
            Aws::CognitoIdentityProvider::Model::SignUpRequest signUpRequest;
            signUpRequest.SetClientId(m_cognitoAppClientId.c_str());
            signUpRequest.SetUsername(username.c_str());
            signUpRequest.SetPassword(password.c_str());

            Aws::Vector<Aws::CognitoIdentityProvider::Model::AttributeType> attributes;
            Aws::CognitoIdentityProvider::Model::AttributeType emailAttribute;
            emailAttribute.SetName("email");
            emailAttribute.SetValue(email.c_str());
            attributes.push_back(emailAttribute);
            signUpRequest.SetUserAttributes(attributes);

            Aws::CognitoIdentityProvider::Model::SignUpOutcome signUpOutcome{ cognitoIdentityProviderClient->SignUp(signUpRequest) };
            if (signUpOutcome.IsSuccess())
            {
                Aws::CognitoIdentityProvider::Model::SignUpResult signUpResult{ signUpOutcome.GetResult() };
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnEmailSignUpSuccess, signUpResult.GetUserSub().c_str());
                
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = signUpOutcome.GetError();
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnEmailSignUpFail, error.GetMessage().c_str());
            }
        }, true, jobContext);
        emailSignUpJob->Start();
    }

    void AWSCognitoUserManagementController::PhoneSignUpAsync(const AZStd::string& username, const AZStd::string& password, const AZStd::string& phoneNumber)
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);

        AZ::Job* phoneSignUpJob = AZ::CreateJobFunction([this, cognitoIdentityProviderClient, username, password, phoneNumber]()
        {
            Aws::CognitoIdentityProvider::Model::SignUpRequest signUpRequest;
            signUpRequest.SetClientId(m_cognitoAppClientId.c_str());
            signUpRequest.SetUsername(username.c_str());
            signUpRequest.SetPassword(password.c_str());

            Aws::Vector<Aws::CognitoIdentityProvider::Model::AttributeType> attributes;
            Aws::CognitoIdentityProvider::Model::AttributeType emailAttribute;
            emailAttribute.SetName("phone_number");
            emailAttribute.SetValue(phoneNumber.c_str());
            attributes.push_back(emailAttribute);
            signUpRequest.SetUserAttributes(attributes);

            Aws::CognitoIdentityProvider::Model::SignUpOutcome signUpOutcome{ cognitoIdentityProviderClient->SignUp(signUpRequest) };
            if (signUpOutcome.IsSuccess())
            {
                Aws::CognitoIdentityProvider::Model::SignUpResult signUpResult{ signUpOutcome.GetResult() };
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnPhoneSignUpSuccess, signUpResult.GetUserSub().c_str());

            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = signUpOutcome.GetError();
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnPhoneSignUpFail, error.GetMessage().c_str());
            }
        }, true, jobContext);
        phoneSignUpJob->Start();
    }

    // Call Cognito user pool confirm sign up using code from email/phone.
    // Refer https://docs.aws.amazon.com/cognito/latest/developerguide/signing-up-users-in-your-app.html
    void AWSCognitoUserManagementController::ConfirmSignUpAsync(const AZStd::string& username, const AZStd::string& confirmationCode)
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();
 
        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);

        AZ::Job* confirmSignUpJob = AZ::CreateJobFunction([this, cognitoIdentityProviderClient, username, confirmationCode]()
        {
            Aws::CognitoIdentityProvider::Model::ConfirmSignUpRequest confirmSignupRequest;
            confirmSignupRequest.SetClientId(m_cognitoAppClientId.c_str());
            confirmSignupRequest.SetUsername(username.c_str());
            confirmSignupRequest.SetConfirmationCode(confirmationCode.c_str());

            Aws::CognitoIdentityProvider::Model::ConfirmSignUpOutcome confirmSignupOutcome{ cognitoIdentityProviderClient->ConfirmSignUp(confirmSignupRequest) };
            if (confirmSignupOutcome.IsSuccess())
            {
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnConfirmSignUpSuccess);
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = confirmSignupOutcome.GetError();
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnConfirmSignUpFail, error.GetMessage().c_str());
            }
        }, true, jobContext);
        confirmSignUpJob->Start();
    }

    void AWSCognitoUserManagementController::ForgotPasswordAsync(const AZStd::string& username)
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);

        AZ::Job* forgotPasswordJob = AZ::CreateJobFunction([this, cognitoIdentityProviderClient, username]()
        {
            Aws::CognitoIdentityProvider::Model::ForgotPasswordRequest forgotPasswordRequest;
            forgotPasswordRequest.SetClientId(m_cognitoAppClientId.c_str());
            forgotPasswordRequest.SetUsername(username.c_str());

            Aws::CognitoIdentityProvider::Model::ForgotPasswordOutcome forgotPasswordOutcome{ cognitoIdentityProviderClient->ForgotPassword(forgotPasswordRequest) };
            if (forgotPasswordOutcome.IsSuccess())
            {
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnForgotPasswordSuccess);
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = forgotPasswordOutcome.GetError();
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnForgotPasswordFail, error.GetMessage().c_str());
            }
        }, true, jobContext);
        forgotPasswordJob->Start();
    }

    void AWSCognitoUserManagementController::ConfirmForgotPasswordAsync(const AZStd::string& username, const AZStd::string& confirmationCode, const AZStd::string& newPassword)
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);

        AZ::Job* confirmForgotPasswordJob = AZ::CreateJobFunction([this, cognitoIdentityProviderClient, username, confirmationCode, newPassword]()
        {
            Aws::CognitoIdentityProvider::Model::ConfirmForgotPasswordRequest confirmForgotPasswordRequest;
            confirmForgotPasswordRequest.SetClientId(m_cognitoAppClientId.c_str());
            confirmForgotPasswordRequest.SetUsername(username.c_str());
            confirmForgotPasswordRequest.SetConfirmationCode(confirmationCode.c_str());
            confirmForgotPasswordRequest.SetPassword(newPassword.c_str());

            Aws::CognitoIdentityProvider::Model::ConfirmForgotPasswordOutcome confirmForgotPasswordOutcome{ cognitoIdentityProviderClient->ConfirmForgotPassword(confirmForgotPasswordRequest) };
            if (confirmForgotPasswordOutcome.IsSuccess())
            {
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnConfirmForgotPasswordSuccess);
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = confirmForgotPasswordOutcome.GetError();
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnConfirmForgotPasswordFail, error.GetMessage().c_str());
            }
        }, true, jobContext);
        confirmForgotPasswordJob->Start();
    }

    void AWSCognitoUserManagementController::EnableMFAAsync(const AZStd::string& accessToken)
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);

        AZ::Job* enableMFAJob = AZ::CreateJobFunction([cognitoIdentityProviderClient, accessToken]()
        {
            Aws::CognitoIdentityProvider::Model::SetUserMFAPreferenceRequest confirmForgotPasswordRequest;
            Aws::CognitoIdentityProvider::Model::SMSMfaSettingsType settings;
            settings.SetEnabled(true);
            settings.SetPreferredMfa(true);
            confirmForgotPasswordRequest.SetSMSMfaSettings(settings);
            confirmForgotPasswordRequest.SetAccessToken(accessToken.c_str());

            Aws::CognitoIdentityProvider::Model::SetUserMFAPreferenceOutcome setUserMFAPreferenceOutcome{ cognitoIdentityProviderClient->SetUserMFAPreference(confirmForgotPasswordRequest) };
            if (setUserMFAPreferenceOutcome.IsSuccess())
            {
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnEnableMFASuccess);
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = setUserMFAPreferenceOutcome.GetError();
                AWSCognitoUserManagementNotificationBus::Broadcast(&AWSCognitoUserManagementNotifications::OnEnableMFAFail, error.GetMessage().c_str());
            }
        }, true, jobContext);
        enableMFAJob->Start();
    }
} // namespace AWSClientAuth
