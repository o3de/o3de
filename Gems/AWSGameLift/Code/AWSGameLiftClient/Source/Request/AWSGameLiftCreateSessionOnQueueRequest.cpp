/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Request/AWSGameLiftCreateSessionOnQueueRequest.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AWSGameLift
{
    void AWSGameLiftCreateSessionOnQueueRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSGameLiftCreateSessionOnQueueRequest, AzFramework::CreateSessionRequest>()
                ->Version(0)
                ->Field("queueName", &AWSGameLiftCreateSessionOnQueueRequest::m_queueName)
                ->Field("placementId", &AWSGameLiftCreateSessionOnQueueRequest::m_placementId)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AWSGameLiftCreateSessionOnQueueRequest>("AWSGameLiftCreateSessionOnQueueRequest", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftCreateSessionOnQueueRequest::m_queueName, "QueueName (Required)",
                        "Name of the queue to use to place the new game session")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftCreateSessionOnQueueRequest::m_placementId, "PlacementId (Required)",
                        "A unique identifier to assign to the new game session placement")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSGameLiftCreateSessionOnQueueRequest>("AWSGameLiftCreateSessionOnQueueRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("CreatorId", BehaviorValueProperty(&AWSGameLiftCreateSessionOnQueueRequest::m_creatorId))
                ->Property("SessionProperties", BehaviorValueProperty(&AWSGameLiftCreateSessionOnQueueRequest::m_sessionProperties))
                ->Property("SessionName", BehaviorValueProperty(&AWSGameLiftCreateSessionOnQueueRequest::m_sessionName))
                ->Property("MaxPlayer", BehaviorValueProperty(&AWSGameLiftCreateSessionOnQueueRequest::m_maxPlayer))
                ->Property("QueueName", BehaviorValueProperty(&AWSGameLiftCreateSessionOnQueueRequest::m_queueName))
                ->Property("PlacementId", BehaviorValueProperty(&AWSGameLiftCreateSessionOnQueueRequest::m_placementId))
                ;
        }
    }
} // namespace AWSGameLift
