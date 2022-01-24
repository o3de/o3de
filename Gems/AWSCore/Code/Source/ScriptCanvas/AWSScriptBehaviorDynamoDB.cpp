/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <aws/core/auth/AWSCredentials.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>
#include <aws/core/utils/Outcome.h>

#include <Framework/AWSApiRequestJob.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <ScriptCanvas/AWSScriptBehaviorDynamoDB.h>

namespace AWSCore
{
    void AWSScriptBehaviorDynamoDB::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSScriptBehaviorDynamoDB>()
                ->Version(0);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSScriptBehaviorDynamoDB>("AWSScriptBehaviorDynamoDB")
                ->Attribute(AZ::Script::Attributes::Category, "AWSCore")
                ->Method("GetItem", &AWSScriptBehaviorDynamoDB::GetItem,
                    {{{"Table Resource KeyName", "The name of the table containing the requested item."},
                      {"Key Map", "A map of attribute names to AttributeValue objects, representing the primary key of the item to retrieve."}}})
                ->Method("GetItemRaw", &AWSScriptBehaviorDynamoDB::GetItemRaw,
                    {{{"Table Name", "The name of the table containing the requested item."},
                      {"Key Map", "A map of attribute names to AttributeValue objects, representing the primary key of the item to retrieve."},
                      {"Region Name", "The region of the table located in."}}});

            behaviorContext->EBus<AWSScriptBehaviorDynamoDBNotificationBus>("AWSDynamoDBBehaviorNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSCore")
                ->Handler<AWSScriptBehaviorDynamoDBNotificationBusHandler>();
        }
    }

    void AWSScriptBehaviorDynamoDB::GetItem(const AZStd::string& tableResourceKey, const DynamoDBAttributeValueMap& keyMap)
    {
        AZStd::string requestTableName = "";
        AWSResourceMappingRequestBus::BroadcastResult(requestTableName, &AWSResourceMappingRequests::GetResourceNameId, tableResourceKey);
        AZStd::string requestRegion = "";
        AWSResourceMappingRequestBus::BroadcastResult(requestRegion, &AWSResourceMappingRequests::GetResourceRegion, tableResourceKey);
        GetItemRaw(requestTableName, keyMap, requestRegion);
    }

    void AWSScriptBehaviorDynamoDB::GetItemRaw(const AZStd::string& table, const DynamoDBAttributeValueMap& keyMap, const AZStd::string& region)
    {
        if (!ValidateGetItemRequest(table, keyMap, region))
        {
            return;
        }

        using DynamoDBGetItemRequestJob = AWS_API_REQUEST_JOB(DynamoDB, GetItem);
        DynamoDBGetItemRequestJob::Config config(DynamoDBGetItemRequestJob::GetDefaultConfig());
        config.region = region.c_str();

        auto job = DynamoDBGetItemRequestJob::Create(
            [](DynamoDBGetItemRequestJob* job) // OnSuccess handler
            {
                auto item = job->result.GetItem();
                if (!item.empty())
                {
                    DynamoDBAttributeValueMap result;
                    for (const auto& itermPair : item)
                    {
                        Aws::String attributeValue = itermPair.second.SerializeAttribute();
                        result.emplace(AZStd::string(itermPair.first.c_str()), AZStd::string(attributeValue.c_str()));
                    }
                    AWSScriptBehaviorDynamoDBNotificationBus::Broadcast(
                        &AWSScriptBehaviorDynamoDBNotificationBus::Events::OnGetItemSuccess, result);
                }
                else
                {
                    AWSScriptBehaviorDynamoDBNotificationBus::Broadcast(
                        &AWSScriptBehaviorDynamoDBNotificationBus::Events::OnGetItemError, "No item was found with the key.");
                }
            },
            [](DynamoDBGetItemRequestJob* job) // OnError handler
            {
                Aws::String errorMessage = job->error.GetMessage();
                AWSScriptBehaviorDynamoDBNotificationBus::Broadcast(
                    &AWSScriptBehaviorDynamoDBNotificationBus::Events::OnGetItemError, errorMessage.c_str());
            },
            &config);

        job->request.SetTableName(table.c_str());
        for (const auto& kepMapPair : keyMap)
        {
            Aws::Utils::Json::JsonValue keyJsonValue(kepMapPair.second.c_str());
            Aws::DynamoDB::Model::AttributeValue keyAttributeValue(keyJsonValue.View());
            job->request.AddKey(kepMapPair.first.c_str(), keyAttributeValue);
        }
        job->Start();
    }

    bool AWSScriptBehaviorDynamoDB::ValidateGetItemRequest(
        const AZStd::string& table, const DynamoDBAttributeValueMap& keyMap, const AZStd::string& region)
    {
        if (table.empty())
        {
            AZ_Warning("AWSScriptBehaviorDynamoDB", false, "Request validation failed, table name is required.");
            AWSScriptBehaviorDynamoDBNotificationBus::Broadcast(
                &AWSScriptBehaviorDynamoDBNotificationBus::Events::OnGetItemError, "Request validation failed, table name is required.");
            return false;
        }
        if (keyMap.empty())
        {
            AZ_Warning("AWSScriptBehaviorDynamoDB", false, "Request validation failed, key is required.");
            AWSScriptBehaviorDynamoDBNotificationBus::Broadcast(
                &AWSScriptBehaviorDynamoDBNotificationBus::Events::OnGetItemError, "Request validation failed, key is required.");
            return false;
        }
        else
        {
            for (const auto& kepMapPair : keyMap)
            {
                Aws::Utils::Json::JsonValue keyJsonValue(kepMapPair.second.c_str());
                if (!keyJsonValue.WasParseSuccessful())
                {
                    AZ_Warning("AWSScriptBehaviorDynamoDB", false, "Request validation failed, key attribute value is invalid.");
                    AWSScriptBehaviorDynamoDBNotificationBus::Broadcast(
                        &AWSScriptBehaviorDynamoDBNotificationBus::Events::OnGetItemError, "Request validation failed, key attribute value is invalid.");
                    return false;
                }
            }
        }
        if (region.empty())
        {
            AZ_Warning("AWSScriptBehaviorDynamoDB", false, "Request validation failed, region name is required.");
            AWSScriptBehaviorDynamoDBNotificationBus::Broadcast(
                &AWSScriptBehaviorDynamoDBNotificationBus::Events::OnGetItemError, "Request validation failed, region name is required.");
            return false;
        }
        return true;
    }
} // namespace AWSCore
