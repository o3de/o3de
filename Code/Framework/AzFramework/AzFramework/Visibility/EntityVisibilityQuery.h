/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Visibility/VisibilityDebug.h>
#include <AzFramework/Viewport/CameraState.h>

namespace AzFramework
{
    //! Utility class to encapsulate entity visibility queries against the view frustum.
    class EntityVisibilityQuery
    {
    public:
        EntityVisibilityQuery() = default;
        EntityVisibilityQuery(const EntityVisibilityQuery&) = default;
        EntityVisibilityQuery& operator=(const EntityVisibilityQuery&) = default;
        EntityVisibilityQuery(EntityVisibilityQuery&&) = default;
        EntityVisibilityQuery& operator=(EntityVisibilityQuery&&) = default;
        ~EntityVisibilityQuery() = default;

        void UpdateVisibility(const AzFramework::CameraState& cameraState);
        void DisplayVisibility(AzFramework::DebugDisplayRequests& debugDisplay);

        using EntityIds = AZStd::vector<AZ::EntityId>;
        using EntityIdIterator = EntityIds::iterator;
        using ConstEntityIdIterator = EntityIds::const_iterator;

        ConstEntityIdIterator Begin() const;
        ConstEntityIdIterator End() const;

    private:
        EntityIds m_visibleEntityIds;
        AZStd::optional<AzFramework::CameraState> m_lastCameraState;
        AzFramework::OctreeDebug m_octreeDebug;
    };

    inline EntityVisibilityQuery::ConstEntityIdIterator EntityVisibilityQuery::Begin() const
    {
        return m_visibleEntityIds.cbegin();
    }

    inline EntityVisibilityQuery::ConstEntityIdIterator EntityVisibilityQuery::End() const
    {
        return m_visibleEntityIds.cend();
    }
} // namespace AzFramework
