#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/Authentication/AuthenticationProviderBus.h
    Include/Authentication/AuthenticationTokens.h
    Include/Authorization/AWSCognitoAuthorizationBus.h
    Include/Authorization/ClientAuthAWSCredentials.h
    Include/UserManagement/AWSCognitoUserManagementBus.h

    Source/AWSClientAuthSystemComponent.cpp
    Source/AWSClientAuthSystemComponent.h
    Source/AWSClientAuthBus.h
    Source/AWSClientAuthResourceMappingConstants.h

    Source/Authentication/AuthenticationNotificationBusBehaviorHandler.h
    Source/Authentication/AuthenticationProviderInterface.cpp
    Source/Authentication/AuthenticationProviderInterface.h
    Source/Authentication/AuthenticationProviderManager.cpp
    Source/Authentication/AuthenticationProviderManager.h
    Source/Authentication/AuthenticationProviderScriptCanvasBus.h
    Source/Authentication/AuthenticationProviderTypes.h
    Source/Authentication/AuthenticationTokens.cpp
    Source/Authentication/AWSCognitoAuthenticationProvider.cpp
    Source/Authentication/AWSCognitoAuthenticationProvider.h
    Source/Authentication/LWAAuthenticationProvider.cpp
    Source/Authentication/LWAAuthenticationProvider.h
    Source/Authentication/GoogleAuthenticationProvider.cpp
    Source/Authentication/GoogleAuthenticationProvider.h  
    Source/Authentication/OAuthConstants.h

    Source/Authorization/AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider.cpp
    Source/Authorization/AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider.h
    Source/Authorization/AWSClientAuthPersistentCognitoIdentityProvider.cpp
    Source/Authorization/AWSClientAuthPersistentCognitoIdentityProvider.h
    Source/Authorization/AWSCognitoAuthorizationController.cpp
    Source/Authorization/AWSCognitoAuthorizationController.h
    Source/Authorization/AWSCognitoAuthorizationNotificationBusBehaviorHandler.h
    Source/Authorization/ClientAuthAWSCredentials.cpp

    Source/UserManagement/AWSCognitoUserManagementController.cpp
    Source/UserManagement/AWSCognitoUserManagementController.h
    Source/UserManagement/UserManagementNotificationBusBehaviorHandler.h
)
