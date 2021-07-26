/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QAction>
#include <QCompleter>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QScopedValueRollback>
#include <QLineEdit>
#include <QTimer>
#include <QPushButton>
#include <QHeaderView>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Entity/EditorEntityContextPickingBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <Editor/View/Widgets/VariablePanel/SlotTypeSelectorWidget.h>
#include <Editor/View/Widgets/VariablePanel/ui_SlotTypeSelectorWidget.h>

#include <Data/Data.h>

#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/QtMetaTypes.h>
#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/DataTypePalette/DataTypePaletteModel.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/PropertyGridBus.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>

namespace ScriptCanvasEditor
{
    ///////////////////////
    // SlotTypeSelectorWidget
    ///////////////////////

    SlotTypeSelectorWidget::SlotTypeSelectorWidget(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDialog(parent)
        , m_manipulatingSelection(false)
        , ui(new Ui::SlotTypeSelectorWidget())
        , m_scriptCanvasId(scriptCanvasId)
    {
        ui->setupUi(this);

        ui->variablePalette->SetActiveScene(scriptCanvasId);

        ui->searchFilter->setClearButtonEnabled(true);
        QObject::connect(ui->searchFilter, &QLineEdit::textChanged, this, &SlotTypeSelectorWidget::OnQuickFilterChanged);
        QObject::connect(ui->slotName, &QLineEdit::returnPressed, this, &SlotTypeSelectorWidget::OnReturnPressed);
        QObject::connect(ui->slotName, &QLineEdit::textChanged, this, &SlotTypeSelectorWidget::OnNameChanged);
        QObject::connect(ui->variablePalette, &QTableView::clicked, this, [this]() { ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true); });
        QObject::connect(ui->variablePalette, &VariablePaletteTableView::CreateNamedVariable, this, [this](const AZStd::string& variableName, const ScriptCanvas::Data::Type& variableType)
            {
                // only emitted on container types
                OnCreateVariable(variableType);
                OnNameChanged(variableName.c_str());
                accept();
            });

        // Tell the widget to auto create our context menu, for now
        setContextMenuPolicy(Qt::ActionsContextMenu);

        ui->searchFilter->setEnabled(true);

        QObject::connect(ui->variablePalette, &VariablePaletteTableView::CreateVariable, this, &SlotTypeSelectorWidget::OnCreateVariable);

        m_filterTimer.setInterval(250);
        m_filterTimer.setSingleShot(true);
        m_filterTimer.stop();

        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &SlotTypeSelectorWidget::UpdateFilter);

        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    }

    SlotTypeSelectorWidget::~SlotTypeSelectorWidget()
    {
    }

    void SlotTypeSelectorWidget::PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes)
    {
        ui->variablePalette->PopulateVariablePalette(objectTypes);
    }

    void SlotTypeSelectorWidget::OnEscape()
    {
        reject();
    }

    void SlotTypeSelectorWidget::focusOutEvent(QFocusEvent* )
    {
        reject();
    }

    const ScriptCanvas::ScriptCanvasId& SlotTypeSelectorWidget::GetActiveScriptCanvasId() const
    {
        return m_scriptCanvasId;
    }

    void SlotTypeSelectorWidget::ShowVariablePalette()
    {
        ClearFilter();

        ui->searchFilter->setPlaceholderText("Variable Type...");
        FocusOnSearchFilter();

        ui->searchFilter->setCompleter(ui->variablePalette->GetVariableCompleter());

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void SlotTypeSelectorWidget::FocusOnSearchFilter()
    {
        ui->searchFilter->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void SlotTypeSelectorWidget::ClearFilter()
    {
        {
            QSignalBlocker blocker(ui->searchFilter);
            ui->searchFilter->setText("");
        }

        UpdateFilter();
    }

    void SlotTypeSelectorWidget::UpdateFilter()
    {
        ui->variablePalette->SetFilter(ui->searchFilter->userInputText());
    }

    void SlotTypeSelectorWidget::OnReturnPressed()
    {
        // Set the type to the slot
        if (!m_selectedType.IsNull())
        {
            m_slotName = ui->slotName->text().toUtf8().data();
            accept();
        }
    }

    AZ::Uuid SlotTypeSelectorWidget::GetSelectedType() const
    {
        return m_selectedType;
    }

    AZStd::string SlotTypeSelectorWidget::GetSlotName() const
    {
        return m_slotName;
    }

    void SlotTypeSelectorWidget::SetSlotName(AZStd::string name)
    {
        m_slotName = name;
        ui->slotName->setText(QString(name.c_str()));
    }

    void SlotTypeSelectorWidget::OnQuickFilterChanged(const QString& text)
    {
        if (text.isEmpty())
        {
            //If field was cleared, update immediately
            UpdateFilter();
            return;
        }
        m_filterTimer.stop();
        m_filterTimer.start();
    }

    void SlotTypeSelectorWidget::OnNameChanged(const QString& text)
    {
        bool nameInUse = false;

        const AZStd::unordered_map<ScriptCanvas::VariableId, ScriptCanvas::GraphVariable>* properties = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(properties, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariables);

        int numInUse = 0;
        if (properties)
        {
            for (const auto& variablePair : (*properties))
            {
                AZStd::string testName = text.toUtf8().data();
                if (testName.compare(variablePair.second.GetVariableName()) == 0)
                {
                    nameInUse = true;
                    ++numInUse;
                }
            }
        }

        m_slotName = text.toUtf8().data();

        if (nameInUse)
        {
            m_slotName.append(AZStd::string::format(" (%d)", numInUse));
            ui->slotName->setText(m_slotName.c_str());
        }
    }

    void SlotTypeSelectorWidget::OnContextMenuRequested(const QPoint&)
    {
    }

    void SlotTypeSelectorWidget::OnCreateVariable(ScriptCanvas::Data::Type varType)
    {
        m_selectedType = varType.GetAZType();
    }

#include <Editor/View/Widgets/VariablePanel/moc_SlotTypeSelectorWidget.cpp>
}

