/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzManipulatorTestFramework/ActionDispatcher.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>

namespace AzManipulatorTestFramework
{
    //! Dispatches actions immediately to the manipulators.
    class ImmediateModeActionDispatcher : public ActionDispatcher<ImmediateModeActionDispatcher>
    {
        using KeyboardModifier = AzToolsFramework::ViewportInteraction::KeyboardModifier;
        using KeyboardModifiers = AzToolsFramework::ViewportInteraction::KeyboardModifiers;
        using MouseInteractionEvent = AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    public:
        explicit ImmediateModeActionDispatcher(ManipulatorViewportInteraction& viewportManipulatorInteraction);
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

        //! Get the current state of the keyboard modifiers.
        KeyboardModifiers GetKeyboardModifiers() const;

    protected:
        // ActionDispatcher ...
        void EnableSnapToGridImpl() override;
        void DisableSnapToGridImpl() override;
        void GridSizeImpl(float size) override;
        void CameraStateImpl(const AzFramework::CameraState& cameraState) override;
        void MouseLButtonDownImpl() override;
        void MouseLButtonUpImpl() override;
        void MousePositionImpl(const AzFramework::ScreenPoint& position) override;
        void KeyboardModifierDownImpl(const KeyboardModifier& keyModifier) override;
        void KeyboardModifierUpImpl(const KeyboardModifier& keyModifier) override;
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
        ManipulatorViewportInteraction& m_viewportManipulatorInteraction;
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
        keyboardModifiers = GetKeyboardModifiers();
        return this;
    }

    inline AzToolsFramework::ViewportInteraction::KeyboardModifiers ImmediateModeActionDispatcher::GetKeyboardModifiers() const
    {
        return GetMouseInteractionEvent()->m_mouseInteraction.m_keyboardModifiers;
    }
} // namespace AzManipulatorTestFramework
