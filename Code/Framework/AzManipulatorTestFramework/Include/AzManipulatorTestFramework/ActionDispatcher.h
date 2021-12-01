/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/ToString.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzManipulatorTestFramework
{
    //! Base class for derived immediate action dispatchers.
    template<typename DerivedDispatcherT>
    class ActionDispatcher
    {
    public:
        virtual ~ActionDispatcher() = default;

        //! Enable/disable grid snapping.
        DerivedDispatcherT* SetSnapToGrid(bool enabled);
        //! Set the grid size.
        DerivedDispatcherT* GridSize(float size);
        //! Enable/disable sticky select.
        DerivedDispatcherT* SetStickySelect(bool enabled);
        //! Enable/disable action logging.
        DerivedDispatcherT* LogActions(bool logging);
        //! Output a trace debug message.
        template<typename... Args>
        DerivedDispatcherT* Trace(const char* format, const Args&... args);
        //! Set the camera state.
        DerivedDispatcherT* CameraState(const AzFramework::CameraState& cameraState);
        //! Set the left mouse button down.
        DerivedDispatcherT* MouseLButtonDown();
        //! Set the left mouse button up.
        DerivedDispatcherT* MouseLButtonUp();
        //! Set the middle mouse button down.
        DerivedDispatcherT* MouseMButtonDown();
        //! Set the middle mouse button up.
        DerivedDispatcherT* MouseMButtonUp();
        //! Send a double click event.
        DerivedDispatcherT* MouseLButtonDoubleClick();
        //! Set the keyboard modifier button down.
        DerivedDispatcherT* KeyboardModifierDown(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier);
        //! Set the keyboard modifier button up.
        DerivedDispatcherT* KeyboardModifierUp(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier);
        //! Set the mouse position to the specified screen space position.
        DerivedDispatcherT* MousePosition(const AzFramework::ScreenPoint& position);
        //! Expect the selected manipulator to be interacting.
        DerivedDispatcherT* ExpectManipulatorBeingInteracted();
        //! Do not expect the selected manipulator to be interacting.
        DerivedDispatcherT* ExpectManipulatorNotBeingInteracted();
        //! Set the world transform of the specified entity.
        DerivedDispatcherT* SetEntityWorldTransform(AZ::EntityId entityId, const AZ::Transform& transform);
        //! Select the specified entity.
        DerivedDispatcherT* SetSelectedEntity(AZ::EntityId entity);
        //! Select the specified entities.
        DerivedDispatcherT* SetSelectedEntities(const AzToolsFramework::EntityIdList& entities);
        //! Enter component mode for the specified component type's uuid.
        DerivedDispatcherT* EnterComponentMode(const AZ::Uuid& uuid);
        //! Break out to the debugger mid action sequence (note: do not leave uses in production code).
        DerivedDispatcherT* DebugBreak();
        //!  Enter component mode for the specified component type.
        template<typename ComponentT>
        DerivedDispatcherT* EnterComponentMode();

    protected:
        // Actions to be implemented by derived immediate action dispatcher.
        virtual void SetSnapToGridImpl(bool enabled) = 0;
        virtual void SetStickySelectImpl(bool enabled) = 0;
        virtual void GridSizeImpl(float size) = 0;
        virtual void CameraStateImpl(const AzFramework::CameraState& cameraState) = 0;
        virtual void MouseLButtonDownImpl() = 0;
        virtual void MouseLButtonUpImpl() = 0;
        virtual void MouseMButtonDownImpl() = 0;
        virtual void MouseMButtonUpImpl() = 0;
        virtual void MouseLButtonDoubleClickImpl() = 0;
        virtual void MousePositionImpl(const AzFramework::ScreenPoint& position) = 0;
        virtual void KeyboardModifierDownImpl(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier) = 0;
        virtual void KeyboardModifierUpImpl(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier) = 0;
        virtual void ExpectManipulatorBeingInteractedImpl() = 0;
        virtual void ExpectManipulatorNotBeingInteractedImpl() = 0;
        virtual void SetEntityWorldTransformImpl(AZ::EntityId entityId, const AZ::Transform& transform) = 0;
        virtual void SetSelectedEntityImpl(AZ::EntityId entity) = 0;
        virtual void SetSelectedEntitiesImpl(const AzToolsFramework::EntityIdList& entities) = 0;
        virtual void EnterComponentModeImpl(const AZ::Uuid& uuid) = 0;
        template<typename... Args>
        void Log(const char* format, const Args&... args);
        bool m_logging = false;

    private:
        const char* KeyboardModifierString(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier);
        static void DebugPrint(const char* format, ...);
    };

    template<typename DerivedDispatcherT>
    void ActionDispatcher<DerivedDispatcherT>::DebugPrint(const char* format, ...)
    {
        va_list args;
        va_start(args, format);

        vprintf(format, args);
        printf("\n");

        va_end(args);
    }

    template<typename DerivedDispatcherT>
    template <typename... Args>
    void ActionDispatcher<DerivedDispatcherT>::Log(const char* format, const Args&... args)
    {
        if (m_logging)
        {
            DebugPrint(format, args...);
        }
    }

    template<typename DerivedDispatcherT>
    template <typename... Args>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::Trace(const char* format, const Args&... args)
    {
        if (m_logging)
        {
            DebugPrint(format, args...);
        }

        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::SetSnapToGrid(const bool enabled)
    {
        Log("SnapToGrid %s", enabled ? "on" : "off");
        SetSnapToGridImpl(enabled);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::SetStickySelect(bool enabled)
    {
        Log("StickySelect %s", enabled ? "on" : "off");
        SetStickySelectImpl(enabled);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::GridSize(float size)
    {
        Log("GridSize: %.3f", size);
        GridSizeImpl(size);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::LogActions(bool logging)
    {
        m_logging = logging;
        Log("Log actions: %s", m_logging ? "enabled" : "disabled");
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::CameraState(const AzFramework::CameraState& cameraState)
    {
        Log("Camera state: p(%.3f, %.3f, %.3f) d(%.3f, %.3f, %.3f)", cameraState.m_position.GetX(), cameraState.m_position.GetY(),
            cameraState.m_position.GetZ(), cameraState.m_forward.GetX(), cameraState.m_forward.GetY(), cameraState.m_forward.GetZ());
        CameraStateImpl(cameraState);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MouseLButtonDown()
    {
        Log("Mouse left button down");
        MouseLButtonDownImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MouseLButtonUp()
    {
        Log("Mouse left button up");
        MouseLButtonUpImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MouseMButtonDown()
    {
        Log("Mouse middle button down");
        MouseMButtonDownImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MouseMButtonUp()
    {
        Log("Mouse middle button up");
        MouseMButtonUpImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MouseLButtonDoubleClick()
    {
        Log("Mouse left button double click");
        MouseLButtonDoubleClickImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    const char* ActionDispatcher<DerivedDispatcherT>::KeyboardModifierString(
        const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier)
    {
        using namespace AzToolsFramework::ViewportInteraction;
        switch (keyModifier)
        {
        case KeyboardModifier::Alt:
            return "Alt";
        case KeyboardModifier::Control:
            return "Ctrl";
        case KeyboardModifier::Shift:
            return "Shift";
        case KeyboardModifier::None:
            return "None";
        default:
            return "Unknown modifier";
        }
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::KeyboardModifierDown(
        const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier)
    {
        Log("Keyboard modifier down: %s", KeyboardModifierString(keyModifier));
        KeyboardModifierDownImpl(keyModifier);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::KeyboardModifierUp(
        const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier)
    {
        Log("Keyboard modifier up: %s", KeyboardModifierString(keyModifier));
        KeyboardModifierUpImpl(keyModifier);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MousePosition(const AzFramework::ScreenPoint& position)
    {
        Log("Mouse position: (%i, %i)", position.m_x, position.m_y);
        MousePositionImpl(position);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::ExpectManipulatorBeingInteracted()
    {
        Log("Expecting manipulator interacting");
        ExpectManipulatorBeingInteractedImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::ExpectManipulatorNotBeingInteracted()
    {
        Log("Not expecting manipulator interacting");
        ExpectManipulatorNotBeingInteractedImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::SetEntityWorldTransform(AZ::EntityId entityId, const AZ::Transform& transform)
    {
        Log("Setting entity world transform: %s", AZ::ToString(transform).c_str());
        SetEntityWorldTransformImpl(entityId, transform);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::SetSelectedEntity(AZ::EntityId entity)
    {
        Log("Selecting entity: %u", static_cast<AZ::u64>(entity));
        SetSelectedEntityImpl(entity);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::SetSelectedEntities(const AzToolsFramework::EntityIdList& entities)
    {
        for (const auto& entity : entities)
        {
            Log("Selecting entity %u", static_cast<AZ::u64>(entity));
        }
        SetSelectedEntitiesImpl(entities);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::EnterComponentMode(const AZ::Uuid& uuid)
    {
        Log("Entering component mode: %s", uuid.ToString<AZStd::string>().c_str());
        EnterComponentModeImpl(uuid);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::DebugBreak()
    {
        Log("Breaking to debugger");
        AZ::Debug::Trace::Break();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template<typename DerivedDispatcherT>
    template<typename ComponentT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::EnterComponentMode()
    {
        EnterComponentMode(AZ::AzTypeInfo<ComponentT>::Uuid());
        return static_cast<DerivedDispatcherT*>(this);
    }
} // namespace AzManipulatorTestFramework
