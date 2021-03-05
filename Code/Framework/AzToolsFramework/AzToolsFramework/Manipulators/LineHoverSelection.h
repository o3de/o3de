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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>

namespace AZ
{
    class EntityComponentIdPair;
}

namespace AzToolsFramework
{
    class LineSegmentSelectionManipulator;

    /// LineSegmentHoverSelection is a concrete implementation of HoverSelection wrapping a collection/container
    /// of vertices and a list of LineSegmentManipulators. The underlying manipulators are used to control selection
    /// by highlighting where on the line a new vertex will be inserted.
    template<typename Vertex>
    class LineSegmentHoverSelection
        : public HoverSelection
    {
    public:
        explicit LineSegmentHoverSelection(
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            ManipulatorManagerId managerId);
        LineSegmentHoverSelection(const LineSegmentHoverSelection&) = delete;
        LineSegmentHoverSelection& operator=(const LineSegmentHoverSelection&) = delete;
        ~LineSegmentHoverSelection();

        void Register(ManipulatorManagerId managerId) override;
        void Unregister() override;
        void SetBoundsDirty() override;
        void Refresh() override;
        void SetSpace(const AZ::Transform& worldFromLocal) override;

    private:
        AZ::EntityId m_entityId;
        AZStd::vector<AZStd::shared_ptr<LineSegmentSelectionManipulator>> m_lineSegmentManipulators; ///< Manipulators for each line.
    };
} // namespace AzToolsFramework