/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <QApplication>
#include <QCursor>
#include <QMainWindow>
#include <QMouseEvent>
#include <QWidget>

// MSVC: warning suppressed: constant used in conditional expression
// CLANG: warning suppressed: implicit conversion loses integer precision
AZ_PUSH_DISABLE_WARNING(4127, "-Wshorten-64-to-32")
#include <QtTest/qtest.h>
AZ_POP_DISABLE_WARNING

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{
    //////////////////////////////
    // SimulateMouseButtonAction
    //////////////////////////////

    SimulateMouseButtonAction::SimulateMouseButtonAction(MouseAction mouseAction, Qt::MouseButton mouseButton)
        : m_mouseAction(mouseAction)
        , m_mouseButton(mouseButton)
    {

    }

    void SimulateMouseButtonAction::SetTarget(QWidget* targetDispatch)
    {
        m_targetDispatch = targetDispatch;
    }

    bool SimulateMouseButtonAction::Tick()
    {
#if defined(AZ_COMPILER_MSVC)
        INPUT osInput = { 0 };
        osInput.type = INPUT_MOUSE;

        osInput.mi.mouseData = 0;
        osInput.mi.time = 0;
        
        switch (m_mouseAction)
        {
        case MouseAction::Press:
            switch (m_mouseButton)
            {
            case Qt::MouseButton::LeftButton:
                osInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                break;
            case Qt::MouseButton::RightButton:
                osInput.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                break;
            }
            break;
        case MouseAction::Release:
            switch (m_mouseButton)
            {
            case Qt::MouseButton::LeftButton:
                osInput.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                break;
            case Qt::MouseButton::RightButton:
                osInput.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                break;
            }
            break;
        default:
            break;
        }

        ::SendInput(1, &osInput, sizeof(INPUT));
#endif
        return true;
    }

    /////////////////////
    // MouseClickAction
    /////////////////////

    MouseClickAction::MouseClickAction(Qt::MouseButton mouseButton)
        : CompoundAction()
        , m_mouseButton(mouseButton)
        , m_hasFixedTarget(false)
    {
        PopulateActionQueue();
    }
    
    MouseClickAction::MouseClickAction(Qt::MouseButton mouseButton, QPoint cursorPosition)
        : CompoundAction()
        , m_mouseButton(mouseButton)
        , m_hasFixedTarget(true)
        , m_cursorPosition(cursorPosition)
    {
        PopulateActionQueue();
    }
    
    bool MouseClickAction::IsMissingPrecondition()
    {
        if (m_hasFixedTarget)
        {
            QPoint screenPoint = QCursor::pos();

            // Need to give this a little 'wiggle' room.
            QRectF clickArea = QRectF(screenPoint, screenPoint);
            clickArea.adjust(-2, -2, 2, 2);

            return !clickArea.contains(m_cursorPosition);
        }
        
        return false;
    }
    
    EditorAutomationAction* MouseClickAction::GenerateMissingPreconditionAction()
    {
        return aznew MouseMoveAction(m_cursorPosition);
    }

    void MouseClickAction::PopulateActionQueue()
    {
        AddAction(aznew DelayAction(AZStd::chrono::milliseconds(500)));
        AddAction(aznew PressMouseButtonAction(m_mouseButton));
        AddAction(aznew DelayAction(AZStd::chrono::milliseconds(10)));
        AddAction(aznew ReleaseMouseButtonAction(m_mouseButton));
        AddAction(aznew ProcessUserEventsAction());
    }
    
    ////////////////////
    // MouseMoveAction
    ////////////////////
    
    MouseMoveAction::MouseMoveAction(QPoint targetPosition, int ticks)
        : m_tickDuration(ticks)
        , m_targetPosition(targetPosition)
    {
    }

    void MouseMoveAction::SetupAction()
    {
        m_tickCount = 0;
        m_hasStartPosition = false;
    }
    
    bool MouseMoveAction::Tick()
    {
        ++m_tickCount;

        if (!m_hasStartPosition)
        {
            m_startPosition = QCursor::pos();
        }

        QPointF targetPoint = m_targetPosition;
        
        float percentage = aznumeric_cast<float>(m_tickCount)/aznumeric_cast<float>(m_tickDuration);
        
        if (!AZ::IsClose(percentage, 1.0f, AZ::Constants::FloatEpsilon))
        {
            int lerpedX = aznumeric_cast<int>((m_targetPosition.x() - m_startPosition.x()) * percentage) + m_startPosition.x();
            int lerpedY = aznumeric_cast<int>((m_targetPosition.y() - m_startPosition.y()) * percentage) + m_startPosition.y();

            targetPoint = QPointF(lerpedX, lerpedY);
        }

#if defined(AZ_COMPILER_MSVC)
        INPUT osInput = { 0 };
        QPointF currentPosition = QCursor::pos();

        osInput.type = INPUT_MOUSE;
        osInput.mi.mouseData = 0;
        osInput.mi.time = 0;
        osInput.mi.dx = static_cast<LONG>(targetPoint.x() - currentPosition.x());
        osInput.mi.dy = static_cast<LONG>(targetPoint.y() - currentPosition.y());
        osInput.mi.dwFlags = MOUSEEVENTF_MOVE;

        ::SendInput(1, &osInput, sizeof(osInput));
#endif

        return AZ::IsClose(percentage, 1.0f, AZ::Constants::FloatEpsilon);
    }

    ////////////////////
    // MouseDragAction
    ////////////////////

    MouseDragAction::MouseDragAction(QPoint startPosition, QPoint endPosition, Qt::MouseButton holdButton)
        : m_holdButton(holdButton)
        , m_startPosition(startPosition)
        , m_endPosition(endPosition)
    {
        AddAction(aznew MouseMoveAction(m_startPosition));
        AddAction(aznew PressMouseButtonAction(m_holdButton));
        AddAction(aznew ProcessUserEventsAction());
        AddAction(aznew MouseMoveAction(m_endPosition));
        AddAction(aznew ProcessUserEventsAction());
        AddAction(aznew ReleaseMouseButtonAction(m_holdButton));
        AddAction(aznew ProcessUserEventsAction());
    }
}
