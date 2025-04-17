/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BaseManipulator.h"
#include "LinearManipulator.h"

#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    //! MultiLinearManipulator serves as a visual tool for users to modify values
    //! in one or more dimensions on axes defined in 3D space.
    class MultiLinearManipulator
        : public BaseManipulator
        , public ManipulatorSpaceWithLocalTransform
    {
        //! Private constructor.
        explicit MultiLinearManipulator(const AZ::Transform& worldFromLocal);

    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(MultiLinearManipulator, "{8490E883-8CC6-44C7-B2FE-AF9C9AF38AA7}", BaseManipulator)

        MultiLinearManipulator() = delete;
        MultiLinearManipulator(const MultiLinearManipulator&) = delete;
        MultiLinearManipulator& operator=(const MultiLinearManipulator&) = delete;

        ~MultiLinearManipulator();

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<MultiLinearManipulator> MakeShared(const AZ::Transform& worldFromLocal);

        //! Mouse action data used by MouseActionCallback
        //! Provides a collection of LinearManipulator actions for each axis.
        struct Action
        {
            AZStd::vector<LinearManipulator::Action> m_actions;
            int m_viewportId; //!< The id of the viewport this manipulator is being used in.
        };

        //! This is the function signature of callbacks that will be invoked whenever a MultiLinearManipulator
        //! is clicked on or dragged.
        using MouseActionCallback = AZStd::function<void(const Action&)>;
        using InvalidateActionCallback = AZStd::function<void()>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);
        void InstallInvalidateCallback(const InvalidateActionCallback& onInvalidateCallback);

        // BaseManipulator ...
        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void AddAxis(const AZ::Vector3& axis);
        void AddAxes(const AZStd::vector<AZ::Vector3>& axes);
        void ClearAxes();

        using ConstFixedIterator = AZStd::vector<LinearManipulator::Fixed>::const_iterator;
        ConstFixedIterator FixedBegin() const;
        ConstFixedIterator FixedEnd() const;

        template<typename Views>
        void SetViews(Views&& views)
        {
            m_manipulatorViews = AZStd::forward<Views>(views);
        }

    private:
        void OnLeftMouseDownImpl(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        AZStd::vector<LinearManipulator::Fixed> m_fixedAxes; //!< A collection of LinearManipulator fixed states.
        AZStd::vector<LinearManipulator::Starter> m_starters; //!< A collection of LinearManipulator starter states.

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onMouseMoveCallback = nullptr;
        InvalidateActionCallback m_onInvalidateCallback = nullptr;

        ManipulatorViews m_manipulatorViews; //!< Look of manipulator.
    };

    inline MultiLinearManipulator::ConstFixedIterator MultiLinearManipulator::FixedBegin() const
    {
        return m_fixedAxes.cbegin();
    }

    inline MultiLinearManipulator::ConstFixedIterator MultiLinearManipulator::FixedEnd() const
    {
        return m_fixedAxes.cend();
    }
} // namespace AzToolsFramework
