/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzManipulatorTestFramework/ActionDispatcher.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzManipulatorTestFramework
{
    //! Dispatches actions immediately to the manipulators.
    class ImmediateModeActionDispatcher
        : public ActionDispatcher<ImmediateModeActionDispatcher>
        , public AzToolsFramework::ViewportInteraction::EditorModifierKeyRequestBus::Handler
        , public AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequestBus::Handler
    {
        using KeyboardModifier = AzToolsFramework::ViewportInteraction::KeyboardModifier;
        using KeyboardModifiers = AzToolsFramework::ViewportInteraction::KeyboardModifiers;
        using MouseInteractionEvent = AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    public:
        explicit ImmediateModeActionDispatcher(ManipulatorViewportInteraction& manipulatorViewportInteraction);
        ~ImmediateModeActionDispatcher();

        //! Clear the current event state.
        ImmediateModeActionDispatcher* ResetEvent();
        //! Expect the expression to be true.
        ImmediateModeActionDispatcher* ExpectTrue(bool result);
        //! Expect the expression to be false.
        ImmediateModeActionDispatcher* ExpectFalse(bool result);
        //! Expect the two values to be equivalent.
        template<typename ActualT, typename ExpectedT>
        ImmediateModeActionDispatcher* ExpectEq(const ActualT& actual, const ExpectedT& expected);
        //! Expect the value to match the matcher.
        template<typename ValueT, typename MatcherT>
        ImmediateModeActionDispatcher* ExpectThat(const ValueT& value, const MatcherT& matcher);
        //! Get the world transform of the specified entity.
        ImmediateModeActionDispatcher* GetEntityWorldTransform(AZ::EntityId entityId, AZ::Transform& transform);
        //! Get the current state of the keyboard modifiers.
        //! @note Chained version - KeyboardModifiers is returned via an out param.
        ImmediateModeActionDispatcher* GetKeyboardModifiers(KeyboardModifiers& keyboardModifiers);
        //! Execute an arbitrary section of code inline in the action dispatcher.
        ImmediateModeActionDispatcher* ExecuteBlock(const AZStd::function<void()>& blockFn);

        // EditorModifierKeyRequestBus overrides ...
        KeyboardModifiers QueryKeyboardModifiers() override;

        // EditorViewportInputTimeNowRequestBus overrides ...
        AZStd::chrono::milliseconds EditorViewportInputTimeNow() override;

    protected:
        // ActionDispatcher overrides ...
        void SetSnapToGridImpl(bool enabled) override;
        void SetStickySelectImpl(bool enabled) override;
        void GridSizeImpl(float size) override;
        void CameraStateImpl(const AzFramework::CameraState& cameraState) override;
        void MouseLButtonDownImpl() override;
        void MouseLButtonUpImpl() override;
        void MouseMButtonDownImpl() override;
        void MouseMButtonUpImpl() override;
        void MouseRButtonDownImpl() override;
        void MouseRButtonUpImpl() override;
        void MouseLButtonDoubleClickImpl() override;
        void MousePositionImpl(const AzFramework::ScreenPoint& position) override;
        void KeyboardModifierDownImpl(KeyboardModifier keyModifier) override;
        void KeyboardModifierUpImpl(KeyboardModifier keyModifier) override;
        void ExpectManipulatorBeingInteractedImpl() override;
        void ExpectManipulatorNotBeingInteractedImpl() override;
        void SetEntityWorldTransformImpl(AZ::EntityId entityId, const AZ::Transform& transform) override;
        void SetSelectedEntityImpl(AZ::EntityId entity) override;
        void SetSelectedEntitiesImpl(const AzToolsFramework::EntityIdList& entities) override;
        void EnterComponentModeImpl(const AZ::Uuid& uuid) override;

    private:
        // Zero delta mouse move event to be fired after button down/up
        // note: This is to remain consistent with event behavior in the Editor.
        void MouseMoveAfterButton();

        MouseInteractionEvent* GetMouseInteractionEvent();
        const MouseInteractionEvent* GetMouseInteractionEvent() const;

        mutable AZStd::unique_ptr<MouseInteractionEvent> m_event;
        ManipulatorViewportInteraction& m_manipulatorViewportInteraction;

        //! Current time that ticks up after each call to EditorViewportInputTimeNow.
        AZStd::chrono::milliseconds m_timeNow = AZStd::chrono::milliseconds(0);
    };

    template<typename ActualT, typename ExpectedT>
    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ExpectEq(const ActualT& actual, const ExpectedT& expected)
    {
        Log("Expecting equality");
        EXPECT_EQ(actual, expected);
        return this;
    }

    template<typename ValueT, typename MatcherT>
    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ExpectThat(const ValueT& value, const MatcherT& matcher)
    {
        EXPECT_THAT(value, matcher);
        return this;
    }

    inline ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::GetKeyboardModifiers(KeyboardModifiers& keyboardModifiers)
    {
        keyboardModifiers = QueryKeyboardModifiers();
        return this;
    }

    inline AzToolsFramework::ViewportInteraction::KeyboardModifiers ImmediateModeActionDispatcher::QueryKeyboardModifiers()
    {
        return GetMouseInteractionEvent()->m_mouseInteraction.m_keyboardModifiers;
    }

    inline AZStd::chrono::milliseconds ImmediateModeActionDispatcher::EditorViewportInputTimeNow()
    {
        // step the time for each call to be greater than the minimum time required for a double click to register
        // note: the time increment is very high to ensure any potential system changes to settings such as double
        // click interval will not be impacted
        m_timeNow += AZStd::chrono::milliseconds(10000);
        return m_timeNow;
    }
} // namespace AzManipulatorTestFramework
