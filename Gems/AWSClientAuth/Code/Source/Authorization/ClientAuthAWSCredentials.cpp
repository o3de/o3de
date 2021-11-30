/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Authorization/ClientAuthAWSCredentials.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/AzStdOnDemandReflection.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/AZStdContainers.inl>

namespace AWSClientAuth
{
    void ClientAuthAWSCredentials::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ClientAuthAWSCredentials>()
                ->Field("AWSAccessKeyId", &ClientAuthAWSCredentials::m_accessKeyId)
                ->Field("AWSSecretKey", &ClientAuthAWSCredentials::m_secretKey)
                ->Field("AWSSessionToken", &ClientAuthAWSCredentials::m_sessionToken);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<ClientAuthAWSCredentials>()
                ->Attribute(AZ::Script::Attributes::Category, "AWSClientAuth")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Constructor()
                ->Constructor<const ClientAuthAWSCredentials&>()
                ->Property(
                    "AWSAccessKeyId", BehaviorValueGetter(&ClientAuthAWSCredentials::m_accessKeyId),
                    BehaviorValueSetter(&ClientAuthAWSCredentials::m_accessKeyId))
                ->Property(
                    "AWSSecretKey", BehaviorValueGetter(&ClientAuthAWSCredentials::m_secretKey),
                    BehaviorValueSetter(&ClientAuthAWSCredentials::m_secretKey))
                ->Property(
                    "AWSSessionToken", BehaviorValueGetter(&ClientAuthAWSCredentials::m_sessionToken),
                    BehaviorValueSetter(&ClientAuthAWSCredentials::m_sessionToken));
        }
    }
} // namespace AWSClientAuth
