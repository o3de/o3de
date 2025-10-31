/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QLineEdit>
#include <QMenu>
#include <QString>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationTest.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ConnectionActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/CreateElementsActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/GraphActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/VariableActions.h>

namespace ScriptCanvas::Developer
{
    DefineStateId(OpenMenuTest_OpenMenuState);

    /**
        EditorAutomationTest to test the general systems. Will open the Developer menu.
    */
    class OpenMenuTest
        : public EditorAutomationTest
    {
        class OpenMenuState : public StaticIdAutomationState<OpenMenuTest_OpenMenuStateId>
        {
        public:
            AZ_CLASS_ALLOCATOR(OpenMenuState, AZ::SystemAllocator)
            OpenMenuState(QMenu* targetMenu)
                : m_targetMenu(targetMenu)
            {
            }

        protected:

            void OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
            {
                m_shownMenu = false;

                if (m_clickAction == nullptr)
                {
                    QPoint targetPoint = m_targetMenu->mapToGlobal(QPoint(15, -10));

                    m_clickAction = aznew MouseClickAction(Qt::MouseButton::LeftButton, targetPoint);
                    actionRunner.AddAction(m_clickAction);
                }

                actionRunner.AddAction(&m_delayAction);

                m_eventConnection = QObject::connect(m_targetMenu, &QMenu::aboutToShow, [this]()
                {
                    this->OnTargetMenuShown();
                });
            }

            void OnStateActionsComplete()
            {
                QObject::disconnect(m_eventConnection);

                if (!m_shownMenu)
                {
                    ReportError("Failed to show the menu");
                }

                delete m_clickAction;
                m_clickAction = nullptr;
            }

            void OnTargetMenuShown()
            {
                m_shownMenu = true;
            }

        private:

            QMenu* m_targetMenu;

            MouseClickAction* m_clickAction = nullptr;
            DelayAction m_delayAction;

            QMetaObject::Connection m_eventConnection;

            bool m_shownMenu = false;
        };

    public:
        OpenMenuTest(QMenu* targetMenu)
            : EditorAutomationTest("Open Menu Test")
        {
            AddState(new OpenMenuState(targetMenu));
        }

        ~OpenMenuTest() = default;
    };

    DefineStateId(WriteToLineEditState);

    class WriteToLineEditState : public StaticIdAutomationState<WriteToLineEditStateId>
    {
    public:
        AZ_CLASS_ALLOCATOR(WriteToLineEditState, AZ::SystemAllocator)
        WriteToLineEditState(QLineEdit* targetEdit, QString targetText)
            : m_writeToLineEdit(targetEdit, targetText)
        {
        }

    protected:

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
        {
            actionRunner.AddAction(&m_writeToLineEdit);
        }

    private:
        WriteToLineEditAction  m_writeToLineEdit;
    };

    /**
        EditorAutomationTest to test the general systems. Will write the target string to the target line edit
    */
    class WriteTextToInput
        : public EditorAutomationTest
    {
    public:
        WriteTextToInput(QLineEdit* targetEdit, QString targetText)
            : EditorAutomationTest(QString("Write '%1' To Input").arg(targetText))
        {
            AddState(new WriteToLineEditState(targetEdit, targetText));
        }

        ~WriteTextToInput() = default;
    };
}
