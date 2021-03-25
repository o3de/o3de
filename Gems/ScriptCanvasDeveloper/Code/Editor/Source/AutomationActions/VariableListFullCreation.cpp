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
#include "precompiled.h"

#include <QMenu>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>

#include <ScriptCanvasDeveloperEditor/AutomationActions/VariableListFullCreation.h>
#include <ScriptCanvasDeveloperEditor/DeveloperUtils.h>

namespace ScriptCanvasDeveloperEditor
{
    namespace VariablePaletteFullCreation
    {
        class VariablePaletteFullCreationInterface
            : public ProcessVariablePaletteInterface
        {
        public:
            VariablePaletteFullCreationInterface()
            {
                m_variableNameFormat = AZ::Uuid::CreateRandom().ToString<AZStd::string>().c_str();
                m_variableNameFormat += " %i";
            }

            ~VariablePaletteFullCreationInterface()
            {
            }

            void SetupInterface([[maybe_unused]] const AZ::EntityId& graphCanvasId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
            {
                m_variableRequests = ScriptCanvas::GraphVariableManagerRequestBus::FindFirstHandler(scriptCanvasId);
            }

            bool ShouldProcessVariableType([[maybe_unused]] const ScriptCanvas::Data::Type& dataType) const
            {
                return true;
            }

            void ProcessVariableType(const ScriptCanvas::Data::Type& dataType)
            {
                AZStd::string variableName = AZStd::string::format(m_variableNameFormat.c_str(), m_variableCounter);

                ScriptCanvas::Datum datum(dataType, ScriptCanvas::Datum::eOriginality::Original);

                AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> outcome = AZ::Failure(AZStd::string());
                m_variableRequests->AddVariable(variableName, datum);

                ++m_variableCounter;
            }

        private:

            AZStd::string m_variableNameFormat;
            int m_variableCounter = 0;

            ScriptCanvas::GraphVariableManagerRequests* m_variableRequests = nullptr;
        };

        void VariablePaletteFullCreationAction()
        {
            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationBegin);

            VariablePaletteFullCreationInterface fullCreationInterface;
            DeveloperUtils::ProcessVariablePalette(fullCreationInterface);

            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationEnd);
        }

        QAction* CreateVariablePaletteFullCreationAction(QMenu* mainMenu)
        {
            QAction* createVariablePaletteAction = nullptr;

            if (mainMenu)
            {
                createVariablePaletteAction = mainMenu->addAction(QAction::tr("Create All Variables"));
                createVariablePaletteAction->setAutoRepeat(false);
                createVariablePaletteAction->setToolTip("Tries to create every variable in the variable palette. All of them. At once.");
                createVariablePaletteAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Shift+j", "Debug|Create Variable Palette")));

                QObject::connect(createVariablePaletteAction, &QAction::triggered, &VariablePaletteFullCreationAction);
            }

            return createVariablePaletteAction;
        }
    }
}
