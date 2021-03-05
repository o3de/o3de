/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/Interface/Interface.h>

namespace Physics
{
    /// This structure is used only for reflecting type vector<RayCastHit> to 
    /// serialize and behavior context. It's not used in the API anywhere
    struct RaycastHitArray
    {
        AZ_TYPE_INFO(RaycastHitArray, "{BAFCC4E7-A06B-4909-B2AE-C89D9E84FE4E}");
        AZStd::vector<Physics::RayCastHit> m_hitArray;
    };

    AZStd::vector<AZStd::pair<AzPhysics::CollisionGroup, AZStd::string>> PopulateCollisionGroups()
    {
        AZStd::vector<AZStd::pair<AzPhysics::CollisionGroup, AZStd::string>> elems;
        const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<Physics::CollisionRequests>::Get()->GetCollisionConfiguration();
        for (const AzPhysics::CollisionGroups::Preset& preset : configuration.m_collisionGroups.GetPresets())
        {
            elems.push_back({ AzPhysics::CollisionGroup(preset.m_name), preset.m_name });
        }
        return elems;
    }


    void RayCastHit::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RayCastRequest>()
                ->Field("Distance", &RayCastRequest::m_distance)
                ->Field("Start", &RayCastRequest::m_start)
                ->Field("Direction", &RayCastRequest::m_direction)
                ->Field("Collision", &RayCastRequest::m_collisionGroup)
                ->Field("QueryType", &RayCastRequest::m_queryType)
                ->Field("MaxResults", &RayCastRequest::m_maxResults)
                ;

            serializeContext->Class<RayCastHit>()
                ->Field("Distance", &RayCastHit::m_distance)
                ->Field("Position", &RayCastHit::m_position)
                ->Field("Normal", &RayCastHit::m_normal)
                ;

            serializeContext->Class<RaycastHitArray>()
                ->Field("HitArray", &RaycastHitArray::m_hitArray)
                ;

            if (auto editContext = azrtti_cast<AZ::EditContext*>(serializeContext->GetEditContext()))
            {
                editContext->Enum<QueryType>("Query Type", "Object types to include in the query")
                    ->Value("Static", QueryType::Static)
                    ->Value("Dynamic", QueryType::Dynamic)
                    ->Value("Static and Dynamic", QueryType::StaticAndDynamic)
                    ;

                editContext->Class<RayCastRequest>("RayCast Request", "Parameters for raycast")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RayCastRequest::m_start, "Start", "Start position of the raycast")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RayCastRequest::m_distance, "Distance", "Length of the raycast")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RayCastRequest::m_direction, "Direction", "Direction of the raycast")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &RayCastRequest::m_collisionGroup, "Collision Group", "The layers to include in the query")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &PopulateCollisionGroups)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &RayCastRequest::m_queryType, "Query Type", "Object types to include in the query")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RayCastRequest::m_maxResults, "Max results", "The Maximum results for this request to return, this is limited by the value set in WorldConfiguration")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RayCastRequest>("RayCastRequest")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                ->Property("Distance", BehaviorValueProperty(&RayCastRequest::m_distance))
                ->Property("Start", BehaviorValueProperty(&RayCastRequest::m_start))
                ->Property("Direction", BehaviorValueProperty(&RayCastRequest::m_direction))
                ->Property("Collision", BehaviorValueProperty(&RayCastRequest::m_collisionGroup))
                // Until enum class support for behavior context is done, expose this as an int
                ->Property("QueryType", [](const RayCastRequest& self) { return static_cast<int>(self.m_queryType); },
                                        [](RayCastRequest& self, int newQueryType) { self.m_queryType = QueryType(newQueryType); })
                ->Property("MaxResults", BehaviorValueProperty(&RayCastRequest::m_maxResults))
                ;

            behaviorContext->Class<RayCastHit>("RayCastHit")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                ->Property("Distance", BehaviorValueProperty(&RayCastHit::m_distance))
                ->Property("Position", BehaviorValueProperty(&RayCastHit::m_position))
                ->Property("Normal", BehaviorValueProperty(&RayCastHit::m_normal))
                ->Property("EntityId", [](RayCastHit& result) { return result.m_body != nullptr ? result.m_body->GetEntityId() : AZ::EntityId(); }, nullptr)
                ;

            behaviorContext->Class<RaycastHitArray>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("HitArray", BehaviorValueProperty(&RaycastHitArray::m_hitArray))
                ;
        }
    }

} // namespace Physics
