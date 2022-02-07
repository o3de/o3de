/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Request/AWSGameLiftJoinSessionRequest.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AWSGameLift
{
    void AWSGameLiftJoinSessionRequest::Reflect(AZ::ReflectContext* context)
    {
        AzFramework::JoinSessionRequest::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSGameLiftJoinSessionRequest, AzFramework::JoinSessionRequest>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AWSGameLiftJoinSessionRequest>("AWSGameLiftJoinSessionRequest", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AzFramework::JoinSessionRequest>("JoinSessionRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                // Expose base type to BehaviorContext, but hide it to be used directly
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ;
            behaviorContext->Class<AWSGameLiftJoinSessionRequest>("AWSGameLiftJoinSessionRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("PlayerData", BehaviorValueProperty(&AWSGameLiftJoinSessionRequest::m_playerData))
                ->Property("PlayerId", BehaviorValueProperty(&AWSGameLiftJoinSessionRequest::m_playerId))
                ->Property("SessionId", BehaviorValueProperty(&AWSGameLiftJoinSessionRequest::m_sessionId))
                ;
        }
    }
} // namespace AWSGameLift
