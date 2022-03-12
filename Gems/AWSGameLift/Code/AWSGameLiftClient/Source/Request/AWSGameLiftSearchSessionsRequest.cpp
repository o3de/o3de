/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Request/AWSGameLiftSearchSessionsRequest.h>
#include <AWSGameLiftSessionConstants.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Session/SessionConfig.h>

namespace AWSGameLift
{
    void AWSGameLiftSearchSessionsRequest::Reflect(AZ::ReflectContext* context)
    {
        AzFramework::SearchSessionsRequest::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSGameLiftSearchSessionsRequest, AzFramework::SearchSessionsRequest>()
                ->Version(0)
                ->Field("aliasId", &AWSGameLiftSearchSessionsRequest::m_aliasId)
                ->Field("fleetId", &AWSGameLiftSearchSessionsRequest::m_fleetId)
                ->Field("location", &AWSGameLiftSearchSessionsRequest::m_location);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AWSGameLiftSearchSessionsRequest>("AWSGameLiftSearchSessionsRequest", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftSearchSessionsRequest::m_aliasId, "AliasId (Required, or FleetId)",
                        "A unique identifier for the alias associated with the fleet to search for active game sessions.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftSearchSessionsRequest::m_fleetId, "FleetId (Required, or AliasId)",
                        "A unique identifier for the fleet to search for active game sessions.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftSearchSessionsRequest::m_location, "Location",
                        "A fleet location to search for game sessions.");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AzFramework::SearchSessionsRequest>("SearchSessionsRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                // Expose base type to BehaviorContext, but hide it to be used directly
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
            behaviorContext->Class<AWSGameLiftSearchSessionsRequest>("AWSGameLiftSearchSessionsRequest")

                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("FilterExpression", BehaviorValueProperty(&AWSGameLiftSearchSessionsRequest::m_filterExpression))
                ->Property("SortExpression", BehaviorValueProperty(&AWSGameLiftSearchSessionsRequest::m_sortExpression))
                ->Property("MaxResult", BehaviorValueProperty(&AWSGameLiftSearchSessionsRequest::m_maxResult))
                ->Property("NextToken", BehaviorValueProperty(&AWSGameLiftSearchSessionsRequest::m_nextToken))
                ->Property("AliasId", BehaviorValueProperty(&AWSGameLiftSearchSessionsRequest::m_aliasId))
                ->Property("FleetId", BehaviorValueProperty(&AWSGameLiftSearchSessionsRequest::m_fleetId))
                ->Property("Location", BehaviorValueProperty(&AWSGameLiftSearchSessionsRequest::m_location));
        }
    }
} // namespace AWSGameLift
