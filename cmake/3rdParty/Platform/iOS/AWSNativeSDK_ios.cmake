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

set(AWSNATIVE_SDK_LIB_PATH ${BASE_PATH}/lib/ios/$<IF:$<CONFIG:Debug>,Debug,Release>)

set(AWSNATIVESDK_CORE_LIBS
    ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-core.a
    ${AWSNATIVE_SDK_LIB_PATH}/libcurl.a
)
set(AWSNATIVESDK_COMMON_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-c-common.a)
set(AWSNATIVESDK_EVENTSTREAM_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-c-event-stream.a)
set(AWSNATIVESDK_CHECKSUMS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-checksums.a)
set(AWSNATIVESDK_COGNITOIDENTITY_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-cognito-identity.a)
set(AWSNATIVESDK_COGNITOIDP_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-cognito-idp.a)
set(AWSNATIVESDK_DEVICEFARM_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-devicefarm.a)
set(AWSNATIVESDK_DYNAMODB_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-dynamodb.a)
set(AWSNATIVESDK_GAMELIFT_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-gamelift.a)
set(AWSNATIVESDK_IDENTITYMANAGEMENT_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-identity-management.a)
set(AWSNATIVESDK_KINESIS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-kinesis.a)
set(AWSNATIVESDK_LAMBDA_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-lambda.a)
set(AWSNATIVESDK_MOBILEANALYTICS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-mobileanalytics.a)
set(AWSNATIVESDK_QUEUES_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-queues.a)
set(AWSNATIVESDK_S3_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-s3.a)
set(AWSNATIVESDK_SNS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-sns.a)
set(AWSNATIVESDK_SQS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-sqs.a)
set(AWSNATIVESDK_STS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-sts.a)

find_library(SECURITY_FRAMEWORK Security)
set(AWSNATIVESDK_BUILD_DEPENDENCIES ${SECURITY_FRAMEWORK})
