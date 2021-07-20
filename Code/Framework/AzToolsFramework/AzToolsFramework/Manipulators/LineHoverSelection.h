/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>

namespace AZ
{
    class EntityComponentIdPair;
}

namespace AzToolsFramework
{
    class LineSegmentSelectionManipulator;

    //! LineSegmentHoverSelection is a concrete implementation of HoverSelection wrapping a collection/container
    //! of vertices and a list of LineSegmentManipulators. The underlying manipulators are used to control selection
    //! by highlighting where on the line a new vertex will be inserted.
    template<typename Vertex>
    class LineSegmentHoverSelection : public HoverSelection
    {
    public:
        explicit LineSegmentHoverSelection(const AZ::EntityComponentIdPair& entityComponentIdPair, ManipulatorManagerId managerId);
        LineSegmentHoverSelection(const LineSegmentHoverSelection&) = delete;
        LineSegmentHoverSelection& operator=(const LineSegmentHoverSelection&) = delete;
        ~LineSegmentHoverSelection();

        void Register(ManipulatorManagerId managerId) override;
        void Unregister() override;
        void SetBoundsDirty() override;
        void Refresh() override;
        void SetSpace(const AZ::Transform& worldFromLocal) override;
        void SetNonUniformScale(const AZ::Vector3& nonUniformScale) override;

    private:
        AZ::EntityId m_entityId;
        AZStd::vector<AZStd::shared_ptr<LineSegmentSelectionManipulator>> m_lineSegmentManipulators; //!< Manipulators for each line.
    };
} // namespace AzToolsFramework
