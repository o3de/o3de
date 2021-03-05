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

set(AWSNATIVESDK_PACKAGE_NAME AWSNativeSDK)
set(AWSNATIVESDK_3RDPARTY_DIRECTORY AWS/AWSNativeSDK)
set(AWSNATIVESDK_PACKAGE_VERSION  1.7.167-az.2)
set(AWSNATIVESDK_COMPILEDEFINITIONS AWS_CUSTOM_MEMORY_MANAGEMENT PLATFORM_SUPPORTS_AWS_NATIVE_SDK)

set(AWSNATIVESDK_MODULES
    Common
    Core
    EventStream
    Checksums
    AcessManagement
    CognitoIdentity
    CognitoIdp
    DeviceFarm
    DynamoDB
    GameLift
    IdentityManagement
    Kinesis
    Lambda
    MobileAnalytics
    Queues
    S3
    SNS
    SQS
    STS
    Transfer
)
foreach(awsmodule ${AWSNATIVESDK_MODULES})
    ly_add_external_target(
        NAME ${awsmodule}
        PACKAGE ${AWSNATIVESDK_PACKAGE_NAME} 
        VERSION ${AWSNATIVESDK_PACKAGE_VERSION}
        3RDPARTY_DIRECTORY ${AWSNATIVESDK_3RDPARTY_DIRECTORY}
        INCLUDE_DIRECTORIES include
        COMPILE_DEFINITIONS ${AWSNATIVESDK_COMPILEDEFINITIONS}
    )
endforeach()

# Then we have some groupings by functionality. We define a function here to make this easier to define
include(CMakeParseArguments)
function(ly_aws_addgroup)
    cmake_parse_arguments(ly_aws_add "" "NAME" "BUILD_DEPENDENCIES" ${ARGN})
    ly_add_external_target(
        NAME ${ly_aws_add_NAME}
        PACKAGE ${AWSNATIVESDK_PACKAGE_NAME} 
        VERSION ${AWSNATIVESDK_PACKAGE_VERSION}
        3RDPARTY_DIRECTORY ${AWSNATIVESDK_3RDPARTY_DIRECTORY}
        INCLUDE_DIRECTORIES include
        COMPILE_DEFINITIONS ${AWSNATIVESDK_COMPILEDEFINITIONS}
        BUILD_DEPENDENCIES ${ly_aws_add_BUILD_DEPENDENCIES}
    )
endfunction()

# And here we define the groupings
ly_aws_addgroup(NAME Dependencies
    BUILD_DEPENDENCIES
        3rdParty::AWSNativeSDK::Checksums
        3rdParty::AWSNativeSDK::Common
        3rdParty::AWSNativeSDK::EventStream
)

ly_aws_addgroup(NAME IdentityMetrics
    BUILD_DEPENDENCIES
        3rdParty::AWSNativeSDK::Dependencies
        3rdParty::AWSNativeSDK::CognitoIdentity
        3rdParty::AWSNativeSDK::CognitoIdp
        3rdParty::AWSNativeSDK::Core
        3rdParty::AWSNativeSDK::IdentityManagement
        3rdParty::AWSNativeSDK::STS
        3rdParty::AWSNativeSDK::MobileAnalytics
)

ly_aws_addgroup(NAME IdentityLambda
    BUILD_DEPENDENCIES
        3rdParty::AWSNativeSDK::Dependencies
        3rdParty::AWSNativeSDK::CognitoIdentity
        3rdParty::AWSNativeSDK::CognitoIdp
        3rdParty::AWSNativeSDK::Core
        3rdParty::AWSNativeSDK::IdentityManagement
        3rdParty::AWSNativeSDK::Lambda
        3rdParty::AWSNativeSDK::STS
)

ly_aws_addgroup(NAME GameLiftClient
    BUILD_DEPENDENCIES
        3rdParty::AWSNativeSDK::Core
        3rdParty::AWSNativeSDK::GameLift
        3rdParty::AWSNativeSDK::Dependencies
)
