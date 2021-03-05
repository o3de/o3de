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

#include <AzToolsFramework/Manipulators/ManipulatorBus.h>

namespace AzToolsFramework
{
    /// HoverSelection provides an interface for manipulator/s offering selection when
    /// the mouse is hovered over a particular bound. This interface is used to represent
    /// a Spline manipulator bound, and a series of LineSegment manipulator bounds.
    /// This generic interface allows EditorVertexSelection to use either Spline or LineSegment selection.
    class HoverSelection
    {
    public:
        virtual ~HoverSelection() = default;

        virtual void Register(ManipulatorManagerId managerId) = 0;
        virtual void Unregister() = 0;
        virtual void SetBoundsDirty() = 0;
        virtual void Refresh() = 0;
        virtual void SetSpace(const AZ::Transform& worldFromLocal) = 0;
    };

    /// NullHoverSelection is used when vertices cannot be inserted. This serves as a no-op
    /// and is used to prevent the need for additional null checks in EditorVertexSelection.
    class NullHoverSelection
        : public HoverSelection
    {
    public:
        NullHoverSelection() = default;
        NullHoverSelection(const NullHoverSelection&) = delete;
        NullHoverSelection& operator=(const NullHoverSelection&) = delete;

        void Register(ManipulatorManagerId /*managerId*/) override {}
        void Unregister() override {}
        void SetBoundsDirty() override {}
        void Refresh() override {}
        void SetSpace(const AZ::Transform& /*worldFromLocal*/) override {}
    };
} // namespace AzToolsFramework