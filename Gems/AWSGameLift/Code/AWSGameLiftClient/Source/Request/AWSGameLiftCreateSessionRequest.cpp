/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Request/AWSGameLiftCreateSessionRequest.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AWSGameLift
{
    void AWSGameLiftCreateSessionRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSGameLiftCreateSessionRequest, AzFramework::CreateSessionRequest>()
                ->Version(0)
                ->Field("aliasId", &AWSGameLiftCreateSessionRequest::m_aliasId)
                ->Field("fleetId", &AWSGameLiftCreateSessionRequest::m_fleetId)
                ->Field("idempotencyToken", &AWSGameLiftCreateSessionRequest::m_idempotencyToken)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AWSGameLiftCreateSessionRequest>("AWSGameLiftCreateSessionRequest", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftCreateSessionRequest::m_aliasId, "AliasId (Required, or FleetId)",
                        "A unique identifier for the alias associated with the fleet to create a game session in")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftCreateSessionRequest::m_fleetId, "FleetId (Required, or AliasId)",
                        "A unique identifier for the fleet to create a game session in")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftCreateSessionRequest::m_idempotencyToken, "IdempotencyToken",
                        "Custom string that uniquely identifies the new game session request")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSGameLiftCreateSessionRequest>("AWSGameLiftCreateSessionRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("CreatorId", BehaviorValueProperty(&AWSGameLiftCreateSessionRequest::m_creatorId))
                ->Property("SessionProperties", BehaviorValueProperty(&AWSGameLiftCreateSessionRequest::m_sessionProperties))
                ->Property("SessionName", BehaviorValueProperty(&AWSGameLiftCreateSessionRequest::m_sessionName))
                ->Property("MaxPlayer", BehaviorValueProperty(&AWSGameLiftCreateSessionRequest::m_maxPlayer))
                ->Property("AliasId", BehaviorValueProperty(&AWSGameLiftCreateSessionRequest::m_aliasId))
                ->Property("FleetId", BehaviorValueProperty(&AWSGameLiftCreateSessionRequest::m_fleetId))
                ->Property("IdempotencyToken", BehaviorValueProperty(&AWSGameLiftCreateSessionRequest::m_idempotencyToken))
                ;
        }
    }
} // namespace AWSGameLift
