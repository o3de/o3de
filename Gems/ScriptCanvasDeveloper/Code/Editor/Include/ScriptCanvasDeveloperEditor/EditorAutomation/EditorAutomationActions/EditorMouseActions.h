/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QPoint>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationAction.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationAction that will simulate a user pressing or releasing a mouse button
    */
    class SimulateMouseButtonAction
        : public EditorAutomationAction
    {
    public:
        AZ_CLASS_ALLOCATOR(SimulateMouseButtonAction, AZ::SystemAllocator);
        AZ_RTTI(SimulateMouseButtonAction, "{89D4E6F5-983C-4877-9A32-0929F7D2BB8E}", EditorAutomationAction);

        enum class MouseAction
        {
            Press,
            Release
        };

        SimulateMouseButtonAction(MouseAction mouseAction, Qt::MouseButton mouseButton = Qt::LeftButton);

        void SetTarget(QWidget* targetDispatch);

        bool Tick() override;

    private:

        QWidget* m_targetDispatch = nullptr;

        [[maybe_unused]] MouseAction m_mouseAction;
        [[maybe_unused]] Qt::MouseButton m_mouseButton = Qt::LeftButton;
        Qt::KeyboardModifiers m_keyboardModifiers = Qt::NoModifier;
    };

    /**
        EditorAutomationAction that will simulate a user pressing a mouse button
    */
    class PressMouseButtonAction
        : public SimulateMouseButtonAction 
    {
    public:
        AZ_CLASS_ALLOCATOR(PressMouseButtonAction, AZ::SystemAllocator);
        AZ_RTTI(PressMouseButtonAction, "{DA89D548-3506-4DB1-BCCF-6C03ABD75FFB}", SimulateMouseButtonAction);
        
        PressMouseButtonAction(Qt::MouseButton mouseButton = Qt::LeftButton)
            : SimulateMouseButtonAction(SimulateMouseButtonAction::MouseAction::Press, mouseButton)
        {
        }
    };

    /**
        EditorAutomationAction that will simulate a user releasing a mouse button
    */
    class ReleaseMouseButtonAction
        : public SimulateMouseButtonAction 
    {
    public:
        AZ_CLASS_ALLOCATOR(ReleaseMouseButtonAction, AZ::SystemAllocator);
        AZ_RTTI(ReleaseMouseButtonAction, "{448D4A65-5BF8-4643-AE31-E979F176C3AF}", SimulateMouseButtonAction);
        
        ReleaseMouseButtonAction(Qt::MouseButton mouseButton = Qt::LeftButton)
            : SimulateMouseButtonAction(SimulateMouseButtonAction::MouseAction::Release, mouseButton)
        {
        }
    };

    /**
        EditorAutomationAction that will simulate a user clicking a mouse button. Can take in a point to click on.
    */
    class MouseClickAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(MouseClickAction, AZ::SystemAllocator);
        AZ_RTTI(MouseClickAction, "{AEA337E8-0B2F-443B-AFFB-5688CB996D8E}", CompoundAction);
        
        MouseClickAction(Qt::MouseButton mouseButton);
        MouseClickAction(Qt::MouseButton mouseButton, QPoint cursorPosition);
        ~MouseClickAction() override = default;
        
        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

    protected:

        void PopulateActionQueue();
        
    private:

        Qt::MouseButton m_mouseButton;
        Qt::KeyboardModifiers m_keyboardModifiers = Qt::NoModifier;
        
        bool m_hasFixedTarget;
        QPoint m_cursorPosition;
    };

    /**
        EditorAutomationAction that will simulate a user moving their mouse to the specified point
    */
    class MouseMoveAction
        : public EditorAutomationAction
    {
    public:
        AZ_CLASS_ALLOCATOR(MouseMoveAction, AZ::SystemAllocator);
        AZ_RTTI(MouseMoveAction, "{AEA337E8-0B2F-443B-AFFB-5688CB996D8E}", EditorAutomationAction);
        
        MouseMoveAction(QPoint targetPosition, int ticks = 20);
        ~MouseMoveAction() override = default;

        void SetupAction() override;
        bool Tick() override;
        
    private:
    
        int m_tickDuration = 0;
        int m_tickCount = 0;

        bool m_hasStartPosition = false;
        QPoint m_startPosition;
        QPoint m_targetPosition;
    };

    /**
        EditorAutomationAction that will simulate a user draging the mouse between the specified points, using the specified button.
    */
    class MouseDragAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(MouseDragAction, AZ::SystemAllocator);
        AZ_RTTI(MouseDragAction, "{0B704B80-1BCB-4396-99A6-C5CAC462A7DB}", CompoundAction);

        MouseDragAction(QPoint startPosition, QPoint endPosition, Qt::MouseButton holdButton = Qt::MouseButton::LeftButton);

    private:

        Qt::MouseButton m_holdButton;

        QPoint m_startPosition;
        QPoint m_endPosition;
    };
}
