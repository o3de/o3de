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
    #Static libs
    set(AWSNATIVE_SDK_LIB_PATH ${BASE_PATH}/lib/mac/$<IF:$<CONFIG:Debug>,Debug,Release>)

    set(AWSNATIVESDK_CORE_LIBS 
        ${AWSNATIVE_SDK_LIB_PATH}/libaws-cpp-sdk-core.a
        curl # The one bundled with the aws sdk in 3rdParty doesn't use the right openssl
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
else()
    # Shared Libs
    set(AWSNATIVE_SDK_SHARED_LIB_PATH ${BASE_PATH}/bin/mac/$<IF:$<CONFIG:Debug>,Debug,Release>)

    set(AWSNATIVESDK_CORE_LIBS
        ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-core.dylib
        curl # The one bundled with the aws sdk in 3rdParty doesn't use the right openssl
    )
    set(AWSNATIVESDK_COMMON_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-c-common.dylib)
    set(AWSNATIVESDK_EVENTSTREAM_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-c-event-stream.dylib)
    set(AWSNATIVESDK_CHECKSUMS_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-checksums.dylib)
    set(AWSNATIVESDK_COGNITOIDENTITY_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-cognito-identity.dylib)
    set(AWSNATIVESDK_COGNITOIDP_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-cognito-idp.dylib)
    set(AWSNATIVESDK_DEVICEFARM_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-devicefarm.dylib)
    set(AWSNATIVESDK_DYNAMODB_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-dynamodb.dylib)
    set(AWSNATIVESDK_GAMELIFT_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-gamelift.dylib)
    set(AWSNATIVESDK_IDENTITYMANAGEMENT_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-identity-management.dylib)
    set(AWSNATIVESDK_KINESIS_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-kinesis.dylib)
    set(AWSNATIVESDK_LAMBDA_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-lambda.dylib)
    set(AWSNATIVESDK_MOBILEANALYTICS_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-mobileanalytics.dylib)
    set(AWSNATIVESDK_QUEUES_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-queues.dylib)
    set(AWSNATIVESDK_S3_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-s3.dylib)
    set(AWSNATIVESDK_SNS_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-sns.dylib)
    set(AWSNATIVESDK_SQS_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-sqs.dylib)
    set(AWSNATIVESDK_STS_LIBS ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-sts.dylib)

    set(AWSNATIVESDK_CORE_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-core.dylib)

    set(AWSNATIVESDK_COMMON_RUNTIME_DEPENDENCIES
        ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-c-common.dylib
        ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-c-common.0unstable.dylib
        ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-c-common.1.0.0.dylib
    )

    set(AWSNATIVESDK_EVENTSTREAM_RUNTIME_DEPENDENCIES
        ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-c-event-stream.dylib
        ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-c-event-stream.0unstable.dylib
        ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-c-event-stream.1.0.0.dylib
    )

    set(AWSNATIVESDK_CHECKSUMS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-checksums.dylib)
    set(AWSNATIVESDK_ACCESSMANAGEMENT_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-access-management.dylib)
    set(AWSNATIVESDK_COGNITOIDENTITY_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-cognito-identity.dylib)
    set(AWSNATIVESDK_COGNITOIDP_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-cognito-idp.dylib)
    set(AWSNATIVESDK_DEVICEFARM_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-devicefarm.dylib)
    set(AWSNATIVESDK_DYNAMODB_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-dynamodb.dylib)
    set(AWSNATIVESDK_GAMELIFT_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-gamelift.dylib)
    set(AWSNATIVESDK_IDENTITYMANAGEMENT_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-identity-management.dylib)
    set(AWSNATIVESDK_KINESIS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-kinesis.dylib)
    set(AWSNATIVESDK_LAMBDA_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-lambda.dylib)
    set(AWSNATIVESDK_MOBILEANALYTICS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-mobileanalytics.dylib)
    set(AWSNATIVESDK_QUEUES_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-queues.dylib)
    set(AWSNATIVESDK_S3_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-s3.dylib)
    set(AWSNATIVESDK_SNS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-sns.dylib)
    set(AWSNATIVESDK_SQS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-sqs.dylib)
    set(AWSNATIVESDK_STS_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-sts.dylib)
    set(AWSNATIVESDK_TRANSFER_RUNTIME_DEPENDENCIES ${AWSNATIVE_SDK_SHARED_LIB_PATH}/libaws-cpp-sdk-transfer.dylib)
endif()

