/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/BaseManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    //! Represents a sphere that can be clicked on to trigger a particular behavior.
    //! For example clicking a preview point to create a translation manipulator.
    class SelectionManipulator
        : public BaseManipulator
        , public ManipulatorSpaceWithLocalPosition
    {
        //! Private constructor.
        SelectionManipulator(const AZ::Transform& worldFromLocal, const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne());

    public:
        AZ_RTTI(SelectionManipulator, "{966F44B7-E287-4C28-9734-5958F1A13A1D}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(SelectionManipulator, AZ::SystemAllocator);

        SelectionManipulator() = delete;
        SelectionManipulator(const SelectionManipulator&) = delete;
        SelectionManipulator& operator=(const SelectionManipulator&) = delete;

        ~SelectionManipulator() = default;

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<SelectionManipulator> MakeShared(
            const AZ::Transform& worldFromLocal, const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne());

        //! This is the function signature of callbacks that will be invoked
        //! whenever a selection manipulator is clicked on.
        using MouseActionCallback = AZStd::function<void(const ViewportInteraction::MouseInteraction&)>;

        void InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback);
        void InstallRightMouseDownCallback(const MouseActionCallback& onMouseDownCallback);
        void InstallRightMouseUpCallback(const MouseActionCallback& onMouseUpCallback);

        void Draw(
            const ManipulatorManagerState& managerState,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState,
            const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        bool Selected() const
        {
            return m_selected;
        }

        void Select()
        {
            m_selected = true;
        }

        void Deselect()
        {
            m_selected = false;
        }

        void ToggleSelected()
        {
            m_selected = !m_selected;
        }

        template<typename Views>
        void SetViews(Views&& views)
        {
            m_manipulatorViews = AZStd::forward<Views>(views);
        }

    private:
        void OnLeftMouseDownImpl(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;
        void OnRightMouseDownImpl(const ViewportInteraction::MouseInteraction& interaction, float rayIntersectionDistance) override;
        void OnRightMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction) override;

        void InvalidateImpl() override;
        void SetBoundsDirtyImpl() override;

        bool m_selected = false;

        MouseActionCallback m_onLeftMouseDownCallback = nullptr;
        MouseActionCallback m_onLeftMouseUpCallback = nullptr;
        MouseActionCallback m_onRightMouseDownCallback = nullptr;
        MouseActionCallback m_onRightMouseUpCallback = nullptr;

        ManipulatorViews m_manipulatorViews; //!< Look of manipulator.
    };
} // namespace AzToolsFramework
