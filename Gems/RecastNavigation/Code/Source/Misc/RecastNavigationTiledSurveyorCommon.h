/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Misc/RecastHelpers.h>

namespace RecastNavigation
{
    //! Common logic for Recast navigation tiled collector components. Recommended use is as a base class.
    //! The method provided are not thread-safe. Synchronize as necessary at the higher level.
    class RecastNavigationTiledSurveyorCommon
    {
    public:
        AZ_RTTI(RecastNavigationTiledSurveyorCommon, "{182D93F8-9E76-409B-9939-6816509A6F52}");
        explicit RecastNavigationTiledSurveyorCommon(bool useEditorScene);
        virtual ~RecastNavigationTiledSurveyorCommon();

        //! A container of shapes and their respective Entity Ids
        using QueryHits = AZStd::vector<AzPhysics::SceneQueryHit>;

        void CollectGeometryWithinVolume(const AZ::Aabb& volume, QueryHits& overlapHits);

        void AppendColliderGeometry(TileGeometry& geometry, const QueryHits& overlapHits, bool debugDrawInputData);

        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometryImpl(
            float tileSize,
            float borderSize,
            const AZ::Aabb& worldVolume,
            bool debugDrawInputData);

        bool m_useEditorScene;
        const char* GetSceneName() const;
    };

} // namespace RecastNavigation
