#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Tests/AWSClientAuthGemMock.h
    Tests/AWSClientAuthGemTest.cpp
    
    Tests/AWSClientAuthSystemComponentTest.cpp
    Tests/Authentication/AuthenticationProviderManagerMock.h
    Tests/Authentication/AuthenticationProviderManagerTest.cpp
    Tests/Authentication/AuthenticationProviderManagerScriptCanvasBusTest.cpp
    Tests/Authentication/AWSCognitoAuthenticationProviderTest.cpp
    Tests/Authentication/LWAAuthenticationProviderTest.cpp
    Tests/Authentication/GoogleAuthenticationProviderTest.cpp
    Tests/Authorization/AWSClientAuthPersistentCognitoIdentityProviderTest.cpp
    Tests/Authorization/AWSCognitoAuthorizationControllerTest.cpp
    Tests/UserManagement/AWSCognitoUserManagementControllerTest.cpp
)
