/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Include/Multiplayer/Physics/PhysicsUtils.h>

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <Multiplayer/NetworkTime/INetworkTime.h>

namespace
{
    template<typename RequestT>
    AzPhysics::SceneQueryHits SceneQueryInternal(const RequestT& request)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (!sceneInterface)
        {
            return {};
        }

        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
        if (sceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return {};
        }

        Multiplayer::INetworkTime* currentNetTime = Multiplayer::GetNetworkTime();

        if(!currentNetTime->IsTimeRewound())
        {
            // If the time is not rewound, we simply execute the scene query as is.
            AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(sceneHandle, &request);
            return result;
        }

        // If the time is rewound, we want to query against rigid bodies present at the same frame ID: the same as the current rewound time is.
        RequestT netSceneQueryRequest = request;
        netSceneQueryRequest.m_filterCallback = [&request, currentFrameId = (uint32_t)currentNetTime->GetHostFrameId()](
                                                 const AzPhysics::SimulatedBody* body, const ::Physics::Shape* shape)
        {
            if (body->GetFrameId() == AzPhysics::SimulatedBody::UndefinedFrameId || body->GetFrameId() == currentFrameId)
            {
                if (request.m_filterCallback)
                {
                    return request.m_filterCallback(body, shape);
                }

                // Overlap filter callbacks return true/false rather than Touch/Block/None
                if constexpr (AZStd::is_same_v<RequestT, AzPhysics::OverlapRequest>)
                {
                    return true;
                }
                else
                {
                    return AzPhysics::SceneQuery::QueryHitType::Touch;
                }
            }

            if constexpr (AZStd::is_same_v<RequestT, AzPhysics::OverlapRequest>)
            {
                return false;
            }
            else
            {
                return AzPhysics::SceneQuery::QueryHitType::None;
            }
        };

        // Execute the scene query modified for the time rewind.
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(sceneHandle, &netSceneQueryRequest);
        return result;
    }
}

namespace Multiplayer
{
    namespace Physics
    {
        AzPhysics::SceneQueryHits RayCast(const AzPhysics::RayCastRequest& request)
        {
            return SceneQueryInternal<AzPhysics::RayCastRequest>(request);
        }

        AzPhysics::SceneQueryHits ShapeCast(const AzPhysics::ShapeCastRequest& request)
        {
            return SceneQueryInternal<AzPhysics::ShapeCastRequest>(request);
        }

        AzPhysics::SceneQueryHits Overlap(const AzPhysics::OverlapRequest& request)
        {
            return SceneQueryInternal<AzPhysics::OverlapRequest>(request);
        }
    } // namespace Physics
} // namespace Multiplayer
