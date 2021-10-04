/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Request/AWSGameLiftStopMatchmakingRequest.h>

namespace AWSGameLift
{
    void AWSGameLiftStopMatchmakingRequest::Reflect(AZ::ReflectContext* context)
    {
        AzFramework::StopMatchmakingRequest::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSGameLiftStopMatchmakingRequest, AzFramework::StopMatchmakingRequest>()
                ->Version(0);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AWSGameLiftStopMatchmakingRequest>("AWSGameLiftStopMatchmakingRequest", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSGameLiftStopMatchmakingRequest>("AWSGameLiftStopMatchmakingRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("TicketId", BehaviorValueProperty(&AWSGameLiftStopMatchmakingRequest::m_ticketId));
        }
    }
} // namespace AWSGameLift
