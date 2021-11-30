/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/InvokeRequest.h>
#include <aws/lambda/model/InvokeResult.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <Framework/AWSApiRequestJob.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <ScriptCanvas/AWSScriptBehaviorLambda.h>

namespace AWSCore
{
    void AWSScriptBehaviorLambda::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSScriptBehaviorLambda>()
                ->Version(0);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSScriptBehaviorLambda>("AWSScriptBehaviorLambda")
                ->Attribute(AZ::Script::Attributes::Category, "AWSCore")
                ->Method("Invoke", &AWSScriptBehaviorLambda::Invoke,
                    {{{"Function Resource KeyName", "The resource key name of the lambda function in resource mapping config file."},
                      {"Payload", "The JSON that you want to provide to your Lambda function as input."}}})
                ->Method("InvokeRaw", &AWSScriptBehaviorLambda::InvokeRaw,
                    {{{"Function Name", "The name of the Lambda function, version, or alias."},
                      {"Payload", "The JSON that you want to provide to your Lambda function as input."},
                      {"Region Name", "The region of the lambda function located in."}}});

            behaviorContext->EBus<AWSScriptBehaviorLambdaNotificationBus>("AWSLambdaBehaviorNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSCore")
                ->Handler<AWSScriptBehaviorLambdaNotificationBusHandler>();
        }
    }

    void AWSScriptBehaviorLambda::Invoke(const AZStd::string& functionResourceKey, const AZStd::string& payload)
    {
        AZStd::string requestFunctionName = "";
        AWSResourceMappingRequestBus::BroadcastResult(requestFunctionName, &AWSResourceMappingRequests::GetResourceNameId, functionResourceKey);
        AZStd::string requestRegion = "";
        AWSResourceMappingRequestBus::BroadcastResult(requestRegion, &AWSResourceMappingRequests::GetResourceRegion, functionResourceKey);
        InvokeRaw(requestFunctionName, payload, requestRegion);
    }

    void AWSScriptBehaviorLambda::InvokeRaw(const AZStd::string& functionName, const AZStd::string& payload, const AZStd::string& region)
    {
        if (!ValidateInvokeRequest(functionName, region))
        {
            return;
        }

        using LambdaInvokeRequestJob = AWS_API_REQUEST_JOB(Lambda, Invoke);
        LambdaInvokeRequestJob::Config config(LambdaInvokeRequestJob::GetDefaultConfig());
        config.region = region.c_str();

        auto job = LambdaInvokeRequestJob::Create(
            [](LambdaInvokeRequestJob* job) // OnSuccess handler
            {
                Aws::IOStream& stream = job->result.GetPayload();
                std::istreambuf_iterator<AZStd::string::value_type> eos;
                AZStd::string content = AZStd::string{std::istreambuf_iterator<AZStd::string::value_type>(stream), eos};
                AWSScriptBehaviorLambdaNotificationBus::Broadcast(
                    &AWSScriptBehaviorLambdaNotificationBus::Events::OnInvokeSuccess, content.c_str());
            },
            [](LambdaInvokeRequestJob* job) // OnError handler
            {
                Aws::String errorMessage = job->error.GetMessage();
                AWSScriptBehaviorLambdaNotificationBus::Broadcast(
                    &AWSScriptBehaviorLambdaNotificationBus::Events::OnInvokeError, errorMessage.c_str());
            },
            &config);

        std::shared_ptr<Aws::StringStream> stream = std::make_shared<Aws::StringStream>();
        *stream << payload.c_str();
        job->request.SetFunctionName(functionName.c_str());
        job->request.SetBody(stream);
        job->Start();
    }

    bool AWSScriptBehaviorLambda::ValidateInvokeRequest(const AZStd::string& functionName, const AZStd::string& region)
    {
        if (functionName.empty())
        {
            AZ_Warning("AWSScriptBehaviorLambda", false, "Request validation failed, bucket name is required.");
            AWSScriptBehaviorLambdaNotificationBus::Broadcast(
                &AWSScriptBehaviorLambdaNotificationBus::Events::OnInvokeError, "Request validation failed, function name is required.");
            return false;
        }
        if (region.empty())
        {
            AZ_Warning("AWSScriptBehaviorLambda", false, "Request validation failed, region name is required.");
            AWSScriptBehaviorLambdaNotificationBus::Broadcast(
                &AWSScriptBehaviorLambdaNotificationBus::Events::OnInvokeError, "Request validation failed, region name is required.");
            return false;
        }
        return true;
    }
} // namespace AWSCore
