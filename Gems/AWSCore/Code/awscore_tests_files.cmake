#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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
