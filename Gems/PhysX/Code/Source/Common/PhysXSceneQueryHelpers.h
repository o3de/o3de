/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include<AzFramework/Physics/Common/PhysicsSceneQueries.h>

//physx sdk includes
#include <PxPhysicsVersion.h>
#include <PxQueryFiltering.h>
#include <PxQueryReport.h>
#include <PxRigidActor.h>

namespace PhysX
{
    class Shape;

    namespace SceneQueryHelpers
    {
        //! Helper function to convert AZ query flags to PhysX.
        physx::PxQueryFlags GetPxQueryFlags(const AzPhysics::SceneQuery::QueryType& queryType);

        //! Helper function to convert AZ hit flags to PhysX.
        physx::PxHitFlags GetPxHitFlags(AzPhysics::SceneQuery::HitFlags hitFlags);

        //! Helper function to convert AZ query hit type to PhysX.
        physx::PxQueryHitType::Enum GetPxHitType(AzPhysics::SceneQuery::QueryHitType hitType);

        //! Helper function to convert from PhysX hit to AZ.
        AzPhysics::SceneQueryHit GetHitFromPxHit(const physx::PxLocationHit& pxHit, const physx::PxActorShape& pxActorShape);
        AzPhysics::SceneQueryHit GetHitFromPxOverlapHit(const physx::PxOverlapHit& pxHit);

        //! Helper function to perform a ray cast against a PxRigidActor.
        //! @param request The ray cast request to perform.
        //! @param actor Pointer to the actor.
        //! @return The closest hit.
        AzPhysics::SceneQueryHit ClosestRayHitAgainstPxRigidActor(const AzPhysics::RayCastRequest& request, physx::PxRigidActor* actor);

        //! Helper function to perform a ray cast against all shapes provided and return the closest hit.
        //! @param request The ray cast request to perform.
        //! @param shapes The list of shapes to cast against.
        //! @param parentTransform The world transform of the parent to the shapes.
        //! @return The closest hit.
        AzPhysics::SceneQueryHit ClosestRayHitAgainstShapes(const AzPhysics::RayCastRequest& request,
            const AZStd::vector<AZStd::shared_ptr<PhysX::Shape>>& shapes, const AZ::Transform& parentTransform);

        //! Helper function to make the filter callback always return Block unless the result is None. 
        //! This is needed for queries where we only need the single closest result.
        AzPhysics::SceneQuery::FilterCallback GetSceneQueryBlockFilterCallback(const AzPhysics::SceneQuery::FilterCallback& filterCallback);

        //! Helper function to convert the Overlap Filter Callback returning bool to a standard Filter Callback returning QueryHitType
        AzPhysics::SceneQuery::FilterCallback GetFilterCallbackFromOverlap(const AzPhysics::SceneQuery::OverlapFilterCallback& overlapFilterCallback);

        //!Helper class, responsible for filtering invalid collision candidates prior to more expensive narrow phase checks
        class PhysXQueryFilterCallback
            : public physx::PxQueryFilterCallback
        {
        public:
            PhysXQueryFilterCallback() = default;
            PhysXQueryFilterCallback(
                const AzPhysics::CollisionGroup& collisionGroup,
                AzPhysics::SceneQuery::FilterCallback filterCallback,
                physx::PxQueryHitType::Enum hitType);

            //Performs game specific entity filtering
            physx::PxQueryHitType::Enum preFilter(
                const physx::PxFilterData& queryFilterData, const physx::PxShape* pxShape,
                const physx::PxRigidActor* actor, physx::PxHitFlags& queryTypes) override;

#if (PX_PHYSICS_VERSION_MAJOR == 5)
            // Unused, we're only pre-filtering at this time
            physx::PxQueryHitType::Enum postFilter(
                const physx::PxFilterData& filterData,
                const physx::PxQueryHit& hit,
                const physx::PxShape* shape,
                const physx::PxRigidActor* actor) override;
#endif

            // Unused, we're only pre-filtering at this time
            physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) override;

        private:
            AzPhysics::SceneQuery::FilterCallback m_filterCallback;
            AzPhysics::CollisionGroup m_collisionGroup;
            physx::PxQueryHitType::Enum m_hitType = physx::PxQueryHitType::eBLOCK;
        };

        //! Callback used to process unbounded overlap scene queries.
        struct UnboundedOverlapCallback : public physx::PxHitCallback<physx::PxOverlapHit>
        {
            // physx::PxHitCallback<physx::PxOverlapHit> ...
            physx::PxAgain processTouches(const physx::PxOverlapHit* buffer, physx::PxU32 numHits) override;
            void finalizeQuery() override;

            const AzPhysics::SceneQuery::UnboundedOverlapHitCallback& m_hitCallback;
            AzPhysics::SceneQueryHits& m_results;

            UnboundedOverlapCallback(const AzPhysics::SceneQuery::UnboundedOverlapHitCallback& hitCallback,
                AZStd::vector<physx::PxOverlapHit>& hitBuffer,
                AzPhysics::SceneQueryHits& hits);
        };
    } // namespace SceneQueryHelpers
}
