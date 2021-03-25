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

set(FILES awscore_tests_files.cmake
    Tests/AWSCoreTest.cpp
    Tests/AWSCoreSystemComponentTest.cpp
    Tests/Configuration/AWSCoreConfigurationTest.cpp
    Tests/Credential/AWSCredentialBusTest.cpp
    Tests/Credential/AWSCVarCredentialHandlerTest.cpp
    Tests/Credential/AWSDefaultCredentialHandlerTest.cpp
    Tests/Framework/AWSApiClientJobConfigTest.cpp
    Tests/Framework/AWSApiJobConfigTest.cpp
    Tests/Framework/HttpRequestJobTest.cpp
    Tests/Framework/JsonObjectHandlerTest.cpp
    Tests/Framework/JsonWriterTest.cpp
    Tests/Framework/RequestBuilderTest.cpp
    Tests/Framework/ServiceClientJobConfigTest.cpp
    Tests/Framework/ServiceJobUtilTest.cpp
    Tests/Framework/ServiceRequestJobTest.cpp
    Tests/Framework/UtilTest.cpp
    Tests/ResourceMapping/AWSResourceMappingManagerTest.cpp
    Tests/ResourceMapping/AWSResourceMappingUtilsTest.cpp
    Tests/ScriptCanvas/AWSScriptBehaviorDynamoDBTest.cpp
    Tests/ScriptCanvas/AWSScriptBehaviorLambdaTest.cpp
    Tests/ScriptCanvas/AWSScriptBehaviorS3Test.cpp
    Tests/ScriptCanvas/AWSScriptBehaviorsComponentTest.cpp
    Tests/TestFramework/AWSCoreFixture.h
)
