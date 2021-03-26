#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Include/Public/Authentication/AuthenticationProviderBus.h
    Include/Public/Authentication/AuthenticationTokens.h
    Include/Public/Authorization/AWSCognitoAuthorizationBus.h
    Include/Public/Authorization/ClientAuthAWSCredentials.h
    Include/Public/UserManagement/AWSCognitoUserManagementBus.h

    Include/Private/AWSClientAuthSystemComponent.h
    Include/Private/AWSClientAuthBus.h
    Include/Private/AWSClientAuthResourceMappingConstants.h
    Include/Private/Authentication/AuthenticationProviderTypes.h
    Include/Private/Authentication/AuthenticationProviderManager.h
    Include/Private/Authentication/AuthenticationNotificationBusBehaviorHandler.h

    Include/Private/Authorization/AWSCognitoAuthorizationController.h
    Include/Private/Authorization/AWSClientAuthPersistentCognitoIdentityProvider.h
    Include/Private/Authorization/AWSCognitoAuthorizationNotificationBusBehaviorHandler.h

    Include/Private/UserManagement/AWSCognitoUserManagementController.h
    Include/Private/UserManagement/UserManagementNotificationBusBehaviorHandler.h
    
    Include/Private/Authentication/AuthenticationProviderInterface.h
    Include/Private/Authentication/OAuthConstants.h
    Include/Private/Authentication/AWSCognitoAuthenticationProvider.h
    Include/Private/Authentication/LWAAuthenticationProvider.h
    Include/Private/Authentication/GoogleAuthenticationProvider.h
 
    Source/AWSClientAuthSystemComponent.cpp
    Source/Authentication/AuthenticationTokens.cpp
    Source/Authentication/AuthenticationProviderInterface.cpp
    Source/Authentication/AuthenticationProviderManager.cpp
    Source/Authentication/AWSCognitoAuthenticationProvider.cpp
    Source/Authentication/LWAAuthenticationProvider.cpp
    Source/Authentication/GoogleAuthenticationProvider.cpp

    Source/Authorization/AWSCognitoAuthorizationController.cpp
    Source/Authorization/AWSClientAuthPersistentCognitoIdentityProvider.cpp

    Source/UserManagement/AWSCognitoUserManagementController.cpp
)
