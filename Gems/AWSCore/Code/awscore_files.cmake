#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/AWSCoreBus.h
    Include/Credential/AWSCredentialBus.h
    Include/Framework/AWSApiClientJob.h
    Include/Framework/AWSApiClientJobConfig.h
    Include/Framework/AWSApiJob.h
    Include/Framework/AWSApiJobConfig.h
    Include/Framework/AWSApiRequestJob.h
    Include/Framework/AWSApiRequestJobConfig.h
    Include/Framework/Error.h
    Include/Framework/HttpRequestJob.h
    Include/Framework/HttpRequestJobConfig.h
    Include/Framework/JsonObjectHandler.h
    Include/Framework/JsonWriter.h
    Include/Framework/RequestBuilder.h
    Include/Framework/ServiceClientJob.h
    Include/Framework/ServiceClientJobConfig.h
    Include/Framework/ServiceJob.h
    Include/Framework/ServiceJobConfig.h
    Include/Framework/ServiceJobUtil.h
    Include/Framework/ServiceRequestJob.h
    Include/Framework/ServiceRequestJobConfig.h
    Include/Framework/Util.h
    Include/ResourceMapping/AWSResourceMappingBus.h
    Include/ScriptCanvas/AWSScriptBehaviorDynamoDB.h
    Include/ScriptCanvas/AWSScriptBehaviorLambda.h
    Include/ScriptCanvas/AWSScriptBehaviorS3.h
    Include/ScriptCanvas/AWSScriptBehaviorsComponent.h

    Source/AWSCoreInternalBus.h
    Source/AWSCoreSystemComponent.cpp
    Source/AWSCoreSystemComponent.h
    Source/Configuration/AWSCoreConfiguration.cpp
    Source/Configuration/AWSCoreConfiguration.h
    Source/Credential/AWSCredentialManager.cpp
    Source/Credential/AWSCredentialManager.h
    Source/Credential/AWSCVarCredentialHandler.cpp
    Source/Credential/AWSCVarCredentialHandler.h
    Source/Credential/AWSDefaultCredentialHandler.cpp
    Source/Credential/AWSDefaultCredentialHandler.h
    Source/ResourceMapping/AWSResourceMappingConstants.h
    Source/ResourceMapping/AWSResourceMappingManager.cpp
    Source/ResourceMapping/AWSResourceMappingManager.h
    Source/ResourceMapping/AWSResourceMappingUtils.cpp
    Source/ResourceMapping/AWSResourceMappingUtils.h
    
    Source/Framework/AWSApiJob.cpp
    Source/Framework/AWSApiJobConfig.cpp
    Source/Framework/Error.cpp
    Source/Framework/HttpRequestJob.cpp
    Source/Framework/HttpRequestJobConfig.cpp
    Source/Framework/JsonObjectHandler.cpp
    Source/Framework/RequestBuilder.cpp
    Source/Framework/ServiceJob.cpp
    Source/Framework/ServiceJobConfig.cpp
    Source/ScriptCanvas/AWSScriptBehaviorDynamoDB.cpp
    Source/ScriptCanvas/AWSScriptBehaviorLambda.cpp
    Source/ScriptCanvas/AWSScriptBehaviorS3.cpp
    Source/ScriptCanvas/AWSScriptBehaviorsComponent.cpp
)
