/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
