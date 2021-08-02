/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AWSCore
{
    using DynamoDBAttributeValueMap = AZStd::unordered_map<AZStd::string, AZStd::string>;

    //! AWS Script Behavior notifications for ScriptCanvas behaviors that interact with AWS DynamoDB
    class AWSScriptBehaviorDynamoDBNotifications
        : public AZ::EBusTraits
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Called when a successful script behavior dynamodb get item call has occurred
        virtual void OnGetItemSuccess(const DynamoDBAttributeValueMap& resultBody) = 0;

        //! Called when script behavior dynamodb get item call has failed
        virtual void OnGetItemError(const AZStd::string& errorBody) = 0;
    };
    using AWSScriptBehaviorDynamoDBNotificationBus = AZ::EBus<AWSScriptBehaviorDynamoDBNotifications>;

    class AWSScriptBehaviorDynamoDBNotificationBusHandler
        : public AWSScriptBehaviorDynamoDBNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            AWSScriptBehaviorDynamoDBNotificationBusHandler, "{476BEB41-5C5E-4C18-9801-170309E658BE}",
            AZ::SystemAllocator, OnGetItemSuccess, OnGetItemError);

        void OnGetItemSuccess(const DynamoDBAttributeValueMap& resultBody) override
        {
            Call(FN_OnGetItemSuccess, resultBody);
        }

        void OnGetItemError(const AZStd::string& errorBody) override
        {
            Call(FN_OnGetItemError, errorBody);
        }
    };

    class AWSScriptBehaviorDynamoDB
    {
    public:
        AZ_RTTI(AWSScriptBehaviorDynamoDB, "{569E74F6-1268-4199-9653-A3B603FC9F4F}");

        AWSScriptBehaviorDynamoDB() = default;
        virtual ~AWSScriptBehaviorDynamoDB() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetItem(const AZStd::string& tableResourceKey, const DynamoDBAttributeValueMap& keyMap);
        static void GetItemRaw(const AZStd::string& table, const DynamoDBAttributeValueMap& keyMap, const AZStd::string& region);

    private:
        static bool ValidateGetItemRequest(
            const AZStd::string& table, const DynamoDBAttributeValueMap& keyMap, const AZStd::string& region);
    };
} // namespace AWSCore
