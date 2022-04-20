/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/Jobs/JobFunction.h>

#include <Authentication/AWSCognitoAuthenticationProvider.h>
#include <Authentication/AuthenticationProviderTypes.h>
#include <Authentication/AuthenticationProviderBus.h>
#include <AWSClientAuthBus.h>
#include <AWSCoreBus.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <AWSClientAuthResourceMappingConstants.h>

#include <aws/cognito-idp/model/InitiateAuthRequest.h>
#include <aws/cognito-idp/model/InitiateAuthResult.h>
#include <aws/cognito-idp/model/RespondToAuthChallengeRequest.h>
#include <aws/cognito-idp/model/RespondToAuthChallengeResult.h>
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/cognito-idp/CognitoIdentityProviderErrors.h>

namespace AWSClientAuth
{
    constexpr char CognitoUsernameKey[] = "USERNAME";
    constexpr char CognitoPasswordKey[] = "PASSWORD";
    constexpr char CognitoRefreshTokenAuthParamKey[] = "REFRESH_TOKEN";
    constexpr char CognitoSmsMfaCodeKey[] = "SMS_MFA_CODE";

    bool AWSCognitoAuthenticationProvider::Initialize()
    {
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(
            m_cognitoAppClientId, &AWSCore::AWSResourceMappingRequests::GetResourceNameId, CognitoAppClientIdResourceMappingKey);
        AZ_Warning("AWSCognitoAuthenticationProvider", !m_cognitoAppClientId.empty(), "Missing Cognito App Client Id from resource mappings. Calls to Cognito will fail.");
        return !m_cognitoAppClientId.empty();
    }


    void AWSCognitoAuthenticationProvider::PasswordGrantSingleFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        InitiateAuthInternalAsync(username, password, [this](Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome initiateAuthOutcome)
        {
            if (initiateAuthOutcome.IsSuccess())
            {
                Aws::CognitoIdentityProvider::Model::InitiateAuthResult initiateAuthResult{ initiateAuthOutcome.GetResult() };
                if (initiateAuthResult.GetChallengeName() == Aws::CognitoIdentityProvider::Model::ChallengeNameType::NOT_SET)
                {
                    Aws::CognitoIdentityProvider::Model::AuthenticationResultType authenticationResult = initiateAuthResult.GetAuthenticationResult();
                    UpdateTokens(authenticationResult);

                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInSuccess,
                        AuthenticationTokens(authenticationResult.GetAccessToken().c_str(),
                            authenticationResult.GetRefreshToken().c_str(), authenticationResult.GetIdToken().c_str(),
                            ProviderNameEnum::AWSCognitoIDP, authenticationResult.GetExpiresIn()));
                }
                else
                {
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInFail
                        , AZStd::string::format("Unexpected Challenge type: %s"
                            , Aws::CognitoIdentityProvider::Model::ChallengeNameTypeMapper::GetNameForChallengeNameType(initiateAuthResult.GetChallengeName()).c_str()));
                }
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = initiateAuthOutcome.GetError();
                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnPasswordGrantSingleFactorSignInFail, error.GetMessage().c_str());
            }
        });
    }

    void AWSCognitoAuthenticationProvider::PasswordGrantMultiFactorSignInAsync(const AZStd::string& username, const AZStd::string& password)
    {
        InitiateAuthInternalAsync(username, password, [this](Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome initiateAuthOutcome)
        {
            if (initiateAuthOutcome.IsSuccess())
            {
                Aws::CognitoIdentityProvider::Model::InitiateAuthResult initiateAuthResult{ initiateAuthOutcome.GetResult() };
                if (initiateAuthResult.GetChallengeName() == Aws::CognitoIdentityProvider::Model::ChallengeNameType::SMS_MFA)
                {
                    Aws::CognitoIdentityProvider::Model::AuthenticationResultType authenticationResult = initiateAuthResult.GetAuthenticationResult();
                    // Call on sign in success for MFA
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnPasswordGrantMultiFactorSignInSuccess);
                    m_session = initiateAuthResult.GetSession().c_str();
                }
                else
                {
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnPasswordGrantMultiFactorSignInFail
                        , AZStd::string::format("Unexpected Challenge type: %s"
                            , Aws::CognitoIdentityProvider::Model::ChallengeNameTypeMapper::GetNameForChallengeNameType(initiateAuthResult.GetChallengeName()).c_str()));
                }
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = initiateAuthOutcome.GetError();
                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnPasswordGrantMultiFactorSignInFail, error.GetMessage().c_str());
            }
        });
    }

    // Call RespondToAuthChallenge for Cognito authentication flow.
    // Refer https://docs.aws.amazon.com/cognito/latest/developerguide/amazon-cognito-user-pools-authentication-flow.html.
    void AWSCognitoAuthenticationProvider::PasswordGrantMultiFactorConfirmSignInAsync(const AZStd::string& username, const AZStd::string& confirmationCode)
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
        AZ::Job* confirmSignInJob = AZ::CreateJobFunction([this, cognitoIdentityProviderClient, confirmationCode, username]()
        {
            // Set Request parameters for SMS Multi factor authentication.
            // Note: Email MFA is no longer supported by Cognito, use SMS as MFA
            Aws::CognitoIdentityProvider::Model::RespondToAuthChallengeRequest respondToAuthChallengeRequest;
            respondToAuthChallengeRequest.SetClientId(m_cognitoAppClientId.c_str());
            respondToAuthChallengeRequest.AddChallengeResponses(CognitoSmsMfaCodeKey, confirmationCode.c_str());
            respondToAuthChallengeRequest.AddChallengeResponses(CognitoUsernameKey, username.c_str());
            respondToAuthChallengeRequest.SetChallengeName(Aws::CognitoIdentityProvider::Model::ChallengeNameType::SMS_MFA);
            respondToAuthChallengeRequest.SetSession(m_session.c_str());

            Aws::CognitoIdentityProvider::Model::RespondToAuthChallengeOutcome respondToAuthChallengeOutcome{ cognitoIdentityProviderClient->RespondToAuthChallenge(respondToAuthChallengeRequest) };
            if (respondToAuthChallengeOutcome.IsSuccess())
            {
                Aws::CognitoIdentityProvider::Model::RespondToAuthChallengeResult respondToAuthChallengeResult{ respondToAuthChallengeOutcome.GetResult() };
                if (respondToAuthChallengeResult.GetChallengeName() == Aws::CognitoIdentityProvider::Model::ChallengeNameType::NOT_SET)
                {
                    Aws::CognitoIdentityProvider::Model::AuthenticationResultType authenticationResult = respondToAuthChallengeResult.GetAuthenticationResult();
                    UpdateTokens(authenticationResult);

                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnPasswordGrantMultiFactorConfirmSignInSuccess,
                        AuthenticationTokens(authenticationResult.GetAccessToken().c_str(),
                            authenticationResult.GetRefreshToken().c_str(), authenticationResult.GetIdToken().c_str(),
                            ProviderNameEnum::AWSCognitoIDP, authenticationResult.GetExpiresIn()));
                }
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = respondToAuthChallengeOutcome.GetError();
                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnPasswordGrantMultiFactorConfirmSignInFail, error.GetMessage().c_str());
            }
        }, true, jobContext);
        confirmSignInJob->Start();
    }

    void AWSCognitoAuthenticationProvider::DeviceCodeGrantSignInAsync()
    {
        AZ_Assert(false, "Not supported");
    }

    void AWSCognitoAuthenticationProvider::DeviceCodeGrantConfirmSignInAsync()
    {
        AZ_Assert(false, "Not supported");
    }

    void AWSCognitoAuthenticationProvider::RefreshTokensAsync()
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);

        AZ::Job* initiateAuthJob = AZ::CreateJobFunction([this, cognitoIdentityProviderClient]()
        {
            // Set Request parameters.
            Aws::CognitoIdentityProvider::Model::InitiateAuthRequest initiateAuthRequest;
            initiateAuthRequest.SetClientId(m_cognitoAppClientId.c_str());
            initiateAuthRequest.SetAuthFlow(Aws::CognitoIdentityProvider::Model::AuthFlowType::REFRESH_TOKEN_AUTH);

            // Set username and password for Password grant/ Initiate Auth flow.
            Aws::Map<Aws::String, Aws::String> authParameters
            {
                {CognitoRefreshTokenAuthParamKey, GetAuthenticationTokens().GetRefreshToken().c_str()}
            };
            initiateAuthRequest.SetAuthParameters(authParameters);

            Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome initiateAuthOutcome{ cognitoIdentityProviderClient->InitiateAuth(initiateAuthRequest) };
            if (initiateAuthOutcome.IsSuccess())
            {
                Aws::CognitoIdentityProvider::Model::InitiateAuthResult initiateAuthResult{ initiateAuthOutcome.GetResult() };
                if (initiateAuthResult.GetChallengeName() == Aws::CognitoIdentityProvider::Model::ChallengeNameType::NOT_SET)
                {
                    Aws::CognitoIdentityProvider::Model::AuthenticationResultType authenticationResult = initiateAuthResult.GetAuthenticationResult();
                    UpdateTokens(authenticationResult);

                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensSuccess,
                        AuthenticationTokens(authenticationResult.GetAccessToken().c_str(),
                            authenticationResult.GetRefreshToken().c_str(), authenticationResult.GetIdToken().c_str(),
                            ProviderNameEnum::AWSCognitoIDP, authenticationResult.GetExpiresIn()));
                }
                else
                {
                    AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensFail
                        , AZStd::string::format("Unexpected Challenge type: %s"
                            , Aws::CognitoIdentityProvider::Model::ChallengeNameTypeMapper::GetNameForChallengeNameType(initiateAuthResult.GetChallengeName()).c_str()));
                }
            }
            else
            {
                Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = initiateAuthOutcome.GetError();
                AuthenticationProviderNotificationBus::Broadcast(&AuthenticationProviderNotifications::OnRefreshTokensFail, error.GetMessage().c_str());
            }
        }, true, jobContext);
        initiateAuthJob->Start();
    }

    // Call InitiateAuth for Cognito authentication flow.
    // Refer https://docs.aws.amazon.com/cognito/latest/developerguide/amazon-cognito-user-pools-authentication-flow.html.
    void AWSCognitoAuthenticationProvider::InitiateAuthInternalAsync(const AZStd::string& username, const AZStd::string& password
        , AZStd::function<void(Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome outcome)> outcomeCallback)
    {
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> cognitoIdentityProviderClient =
            AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient();

        AZ::JobContext* jobContext = nullptr;
        AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);

        AZ::Job* initiateAuthJob = AZ::CreateJobFunction([this, cognitoIdentityProviderClient, username, password, outcomeCallback]()
        {
            // Set Request parameters.
            Aws::CognitoIdentityProvider::Model::InitiateAuthRequest initiateAuthRequest;
            initiateAuthRequest.SetClientId(m_cognitoAppClientId.c_str());
            initiateAuthRequest.SetAuthFlow(Aws::CognitoIdentityProvider::Model::AuthFlowType::USER_PASSWORD_AUTH);

            // Set username and password for Password grant/ Initiate Auth flow.
            Aws::Map<Aws::String, Aws::String> authParameters
            {
                {CognitoUsernameKey, username.c_str()},
                {CognitoPasswordKey, password.c_str()}
            };
            initiateAuthRequest.SetAuthParameters(authParameters);

            Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome initiateAuthOutcome{ cognitoIdentityProviderClient->InitiateAuth(initiateAuthRequest) };
            outcomeCallback(initiateAuthOutcome);
        }, true, jobContext);
        initiateAuthJob->Start();
    }

    void AWSCognitoAuthenticationProvider::UpdateTokens(const Aws::CognitoIdentityProvider::Model::AuthenticationResultType& authenticationResult)
    {
        // Storing authentication tokens in memory can be a security concern. The access token and id token are not actually in use by
        // the authentication provider and shouldn't be stored in the member variable.
        m_authenticationTokens = AuthenticationTokens("", authenticationResult.GetRefreshToken().c_str(),
            "", ProviderNameEnum::AWSCognitoIDP,
            authenticationResult.GetExpiresIn());
    }
} // namespace AWSClientAuth
