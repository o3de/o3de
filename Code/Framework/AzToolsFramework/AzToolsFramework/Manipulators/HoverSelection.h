/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/ManipulatorBus.h>

namespace AzToolsFramework
{
    //! HoverSelection provides an interface for manipulator/s offering selection when
    //! the mouse is hovered over a particular bound. This interface is used to represent
    //! a Spline manipulator bound, and a series of LineSegment manipulator bounds.
    //! This generic interface allows EditorVertexSelection to use either Spline or LineSegment selection.
    class HoverSelection
    {
    public:
        virtual ~HoverSelection() = default;

        virtual void Register(ManipulatorManagerId managerId) = 0;
        virtual void Unregister() = 0;
        virtual void SetBoundsDirty() = 0;
        virtual void Refresh() = 0;
        virtual void SetSpace(const AZ::Transform& worldFromLocal) = 0;
        virtual void SetNonUniformScale(const AZ::Vector3& nonUniformScale) = 0;
    };

    //! NullHoverSelection is used when vertices cannot be inserted. This serves as a no-op
    //! and is used to prevent the need for additional null checks in EditorVertexSelection.
    class NullHoverSelection : public HoverSelection
    {
    public:
        NullHoverSelection() = default;
        NullHoverSelection(const NullHoverSelection&) = delete;
        NullHoverSelection& operator=(const NullHoverSelection&) = delete;

        void Register([[maybe_unused]] ManipulatorManagerId managerId) override
        {
        }

        void Unregister() override
        {
        }

        void SetBoundsDirty() override
        {
        }

        void Refresh() override
        {
        }

        void SetSpace([[maybe_unused]] const AZ::Transform& worldFromLocal) override
        {
        }

        void SetNonUniformScale([[maybe_unused]] const AZ::Vector3& nonUniformScale) override
        {
        }
    };
} // namespace AzToolsFramework
