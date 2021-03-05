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


if (LY_MONOLITHIC_GAME)
    set(AWSNATIVE_SDK_LIB_PATH ${BASE_PATH}/lib/windows/$<IF:$<CONFIG:Debug>,Debug,Release>)
else()
    set(AWSNATIVE_SDK_LIB_PATH ${BASE_PATH}/bin/windows/$<IF:$<CONFIG:Debug>,Debug,Release>)
endif()

set(AWSNATIVESDK_CORE_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-core.lib
    Bcrypt.lib
    Userenv.lib
    Version.lib
    Wininet.lib
    Winhttp.lib
    Ws2_32.lib
)
set(AWSNATIVESDK_COMMON_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-c-common.lib)
set(AWSNATIVESDK_EVENTSTREAM_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-c-event-stream.lib)
set(AWSNATIVESDK_CHECKSUMS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-checksums.lib)
set(AWSNATIVESDK_ACCESSMANAGEMENT_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-access-management.lib)
set(AWSNATIVESDK_COGNITOIDENTITY_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-cognito-identity.lib)
set(AWSNATIVESDK_COGNITOIDP_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-cognito-idp.lib)
set(AWSNATIVESDK_DEVICEFARM_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-devicefarm.lib)
set(AWSNATIVESDK_DYNAMODB_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-dynamodb.lib)
set(AWSNATIVESDK_GAMELIFT_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-gamelift.lib)
set(AWSNATIVESDK_IDENTITYMANAGEMENT_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-identity-management.lib)
set(AWSNATIVESDK_KINESIS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-kinesis.lib)
set(AWSNATIVESDK_LAMBDA_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-lambda.lib)
set(AWSNATIVESDK_MOBILEANALYTICS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-mobileanalytics.lib)
set(AWSNATIVESDK_QUEUES_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-queues.lib)
set(AWSNATIVESDK_S3_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-s3.lib)
set(AWSNATIVESDK_SNS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-sns.lib)
set(AWSNATIVESDK_SQS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-sqs.lib)
set(AWSNATIVESDK_STS_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-sts.lib)
set(AWSNATIVESDK_TRANSFER_LIBS ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-transfer.lib)

if (NOT LY_MONOLITHIC_GAME)

    # Add 'USE_IMPORT_EXPORT' for external linkage
    set(AWSNATIVESDK_COMPILE_DEFINITIONS USE_IMPORT_EXPORT)

    #Shared libs
    set(AWSNATIVESDK_CORE_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-core.dll)
    set(AWSNATIVESDK_COMMON_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-c-common.dll)
    set(AWSNATIVESDK_EVENTSTREAM_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-c-event-stream.dll)
    set(AWSNATIVESDK_CHECKSUMS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-checksums.dll)
    set(AWSNATIVESDK_ACCESSMANAGEMENT_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-access-management.dll)
    set(AWSNATIVESDK_COGNITOIDENTITY_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-cognito-identity.dll)
    set(AWSNATIVESDK_COGNITOIDP_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-cognito-idp.dll)
    set(AWSNATIVESDK_DEVICEFARM_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-devicefarm.dll)
    set(AWSNATIVESDK_DYNAMODB_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-dynamodb.dll)
    set(AWSNATIVESDK_GAMELIFT_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-gamelift.dll)
    set(AWSNATIVESDK_IDENTITYMANAGEMENT_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-identity-management.dll)
    set(AWSNATIVESDK_KINESIS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-kinesis.dll)
    set(AWSNATIVESDK_LAMBDA_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-lambda.dll)
    set(AWSNATIVESDK_MOBILEANALYTICS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-mobileanalytics.dll)
    set(AWSNATIVESDK_QUEUES_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-queues.dll)
    set(AWSNATIVESDK_S3_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-s3.dll)
    set(AWSNATIVESDK_SNS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-sns.dll)
    set(AWSNATIVESDK_SQS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-sqs.dll)
    set(AWSNATIVESDK_STS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-sts.dll)
    set(AWSNATIVESDK_TRANSFER_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_LIB_PATH}/aws-cpp-sdk-transfer.dll)

endif()