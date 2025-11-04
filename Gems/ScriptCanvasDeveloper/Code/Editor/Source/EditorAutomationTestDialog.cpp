/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QScreen>
#include <QTableView>
#include <QVBoxLayout>
#include <QWindow>

#include <AzCore/Casting/numeric_cast.h>

#include <GraphCanvas/Editor/Automation/AutomationUtils.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <Editor/GraphCanvas/AutomationIds.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <EditorAutomationTestDialog.h>
#include <EditorAutomationTests/EditorAutomationTests.h>
#include <EditorAutomationTests/GraphCreationTests.h>
#include <EditorAutomationTests/GroupTests.h>
#include <EditorAutomationTests/InteractionTests.h>
#include <EditorAutomationTests/NodeCreationTests.h>
#include <EditorAutomationTests/VariableTests.h>

namespace ScriptCanvas::Developer
{
    //////////////////
    // TestListModel
    //////////////////

    TestListModel::TestListModel()
        : m_runningIcon("Icons/AssetBrowser/in_progress.gif")
        , m_passedIcon(":/ScriptCanvasEditorResources/Resources/valid_icon.png")
        , m_failedIcon(":/ScriptCanvasEditorResources/Resources/error_icon.png")
    {
    }

    TestListModel::~TestListModel()
    {
        for (EditorAutomationTest* test : m_actionTests)
        {
            delete test;
        }
    }

    QModelIndex TestListModel::index(int row, int column, const QModelIndex&) const
    {
        if (row < 0 || row >= m_actionTests.size())
        {
            return QModelIndex();
        }

        return createIndex(row, column, nullptr);
    }

    QModelIndex TestListModel::parent(const QModelIndex&) const
    {
        return QModelIndex();
    }

    int TestListModel::columnCount(const QModelIndex&) const
    {
        return ColumnIndex::Count;
    }

    int TestListModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(m_actionTests.size());
    }

    QVariant TestListModel::headerData(int, Qt::Orientation, int) const
    {
        return QVariant();
    }

    QVariant TestListModel::data(const QModelIndex& index, int role) const
    {
        EditorAutomationTest* actionTest = FindTestForIndex(index);

        if (actionTest == nullptr)
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            if (index.column() == ColumnIndex::TestName)
            {
                return actionTest->GetTestName();
            }
        }
        else if (role == Qt::DecorationRole)
        {
            if (index.column() == ColumnIndex::TestName)
            {
                if (actionTest->HasRun() || actionTest->IsRunning())
                {
                    if (actionTest->IsRunning())
                    {
                        return m_runningIcon;
                    }
                    else if (actionTest->HasErrors())
                    {
                        return m_failedIcon;
                    }
                    else
                    {
                        return m_passedIcon;
                    }
                }
            }
        }

        return QVariant();
    }

    void TestListModel::AddTest(EditorAutomationTest* actionTest)
    {
        m_actionTests.emplace_back(actionTest);
    }

    EditorAutomationTest* TestListModel::FindTestForIndex(const QModelIndex& index) const
    {
        if (index.isValid())
        {
            return FindTestForRow(index.row());
        }

        return nullptr;
    }

    EditorAutomationTest* TestListModel::FindTestForRow(int row) const
    {
        if (row >= 0 && row < m_actionTests.size())
        {
            return m_actionTests[row];
        }

        return nullptr;
    }

    int TestListModel::FindRowForTest(EditorAutomationTest* actionTest)
    {
        for (int i = 0; i < m_actionTests.size(); ++i)
        {
            if (m_actionTests[i] == actionTest)
            {
                return i;
            }
        }

        return -1;
    }

    void TestListModel::UpdateTestDisplay(EditorAutomationTest* actionTest)
    {
        int row = FindRowForTest(actionTest);

        if (row >= 0)
        {
            dataChanged(index(0, 0, QModelIndex()), index(0, ColumnIndex::Count - 1, QModelIndex()));
        }
    }

    ///////////////////////////////
    // EditorAutomationTestDialog
    ///////////////////////////////

    EditorAutomationTestDialog::EditorAutomationTestDialog(QMainWindow* mainWindow)
        : QDialog()
        , m_scriptCanvasWindow(mainWindow)
    {
        EditorAutomationTestDialogRequestBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        QMenuBar* menuBar = mainWindow->menuBar();

        const QObjectList& objectList = menuBar->children();

        QMenu* targetMenu = nullptr;

        for (QObject* object : objectList)
        {
            QMenu* menu = qobject_cast<QMenu*>(object);

            if (menu && menu->title() == "Developer")
            {
                targetMenu = menu;
            }
        }

        QObject* object = GraphCanvas::AutomationUtils::FindObjectById<QObject>(ScriptCanvasEditor::AssetEditorId, ScriptCanvasEditor::AutomationIds::NodePaletteWidget);

        // Can't use qobject_cast, since it is accessing across DLLs
        GraphCanvas::NodePaletteWidget* nodePaletteWidget = static_cast<GraphCanvas::NodePaletteWidget*>(object);

        if (nodePaletteWidget == nullptr)
        {
            AZ_Error("Editor Automation", false, "Failed to find NodePaletteWidget");
        }

        QLineEdit* searchFilter = nodePaletteWidget->GetSearchFilter();

        setWindowFlag(Qt::WindowType::WindowCloseButtonHint);
        setAttribute(Qt::WA_DeleteOnClose);

        QLayout* layout = new QVBoxLayout();

        m_tableView = new QTableView();
        m_tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_tableView->setMinimumSize(QSize(250, 250));
        m_tableView->setAlternatingRowColors(true);
        m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

        m_testListModel = new TestListModel();

        // Populate tests here

        // General Sanity Tests of the interactions
        m_testListModel->AddTest(aznew OpenMenuTest(targetMenu));
        m_testListModel->AddTest(aznew WriteTextToInput(searchFilter, "Multiply (*)"));
        m_testListModel->AddTest(aznew WriteTextToInput(searchFilter, "::Test::"));
        ////

        // General Test for Graph Creation
        m_testListModel->AddTest(aznew CreateGraphTest());
        m_testListModel->AddTest(aznew CreateFunctionTest());
        ////

        // General Tests for Node Creation
        m_testListModel->AddTest(aznew CreateNodeFromPaletteTest("Multiply (*)", nodePaletteWidget));
        m_testListModel->AddTest(aznew CreateNodeFromPaletteTest("Print", nodePaletteWidget));
        m_testListModel->AddTest(aznew CreateNodeFromContextMenuTest("Multiply (*)"));
        m_testListModel->AddTest(aznew CreateNodeFromContextMenuTest("Print"));

        m_testListModel->AddTest(aznew CreateHelloWorldFromPalette(nodePaletteWidget));
        m_testListModel->AddTest(aznew CreateHelloWorldFromContextMenu());

        m_testListModel->AddTest(aznew CreateExecutionSplicedNodeTest("Build String"));
        m_testListModel->AddTest(aznew CreateDragDropExecutionSpliceNodeTest(nodePaletteWidget, "Build String"));
        ////

        m_testListModel->AddTest(aznew AltClickDeleteTest());

        // Actual BAT tests
        m_testListModel->AddTest(aznew ManuallyCreateVariableTest(ScriptCanvas::Data::Type::Number(), CreateVariableAction::CreationType::AutoComplete));
        m_testListModel->AddTest(aznew ManuallyCreateVariableTest(ScriptCanvas::Data::Type::Number(), CreateVariableAction::CreationType::Palette));
        m_testListModel->AddTest(aznew ManuallyCreateVariableTest(ScriptCanvas::Data::Type::Number(), CreateVariableAction::CreationType::Programmatic));

        m_testListModel->AddTest(aznew ManuallyCreateVariableTest(ScriptCanvas::Data::Type::Vector3(), CreateVariableAction::CreationType::AutoComplete));
        m_testListModel->AddTest(aznew ManuallyCreateVariableTest(ScriptCanvas::Data::Type::Vector3(), CreateVariableAction::CreationType::Palette));
        m_testListModel->AddTest(aznew ManuallyCreateVariableTest(ScriptCanvas::Data::Type::Vector3(), CreateVariableAction::CreationType::Programmatic));

        m_testListModel->AddTest(aznew CreateNamedVariableTest(ScriptCanvas::Data::Type::EntityID(), "Caterpillar", CreateVariableAction::CreationType::AutoComplete));

        m_testListModel->AddTest(aznew DuplicateVariableNameTest(ScriptCanvas::Data::Type::Number(), ScriptCanvas::Data::Type::Number(), "SameType"));
        m_testListModel->AddTest(aznew DuplicateVariableNameTest(ScriptCanvas::Data::Type::Color(), ScriptCanvas::Data::Type::String(), "DifferentType"));

        m_testListModel->AddTest(aznew ModifyNumericInputTest(123.45));
        m_testListModel->AddTest(aznew ModifyStringInputTest("abcdefghijklmnopqrstuvwxyz"));
        m_testListModel->AddTest(aznew ToggleBoolInputTest());


        AZStd::vector< ScriptCanvas::Data::Type > primitiveTypes;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(primitiveTypes, &ScriptCanvasEditor::VariableAutomationRequests::GetPrimitiveTypes);

        m_testListModel->AddTest(aznew VariableLifeCycleTest("Primitive Variable LifeCycle Test", primitiveTypes));

        AZStd::vector< ScriptCanvas::Data::Type > objectTypes;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(objectTypes, &ScriptCanvasEditor::VariableAutomationRequests::GetBehaviorContextObjectTypes);

        m_testListModel->AddTest(aznew VariableLifeCycleTest("BCO Variable LifeCycle Test", objectTypes));

        AZStd::vector< ScriptCanvas::Data::Type > mapTypes;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(mapTypes, &ScriptCanvasEditor::VariableAutomationRequests::GetMapTypes);

        {
            AZStd::vector< ScriptCanvas::Data::Type > tempTypes;

            for (int i = 0; i < AZStd::GetMin(aznumeric_cast<int>(mapTypes.size()), 10); ++i)
            {
                tempTypes.emplace_back((*mapTypes.begin()));
                mapTypes.erase(mapTypes.begin());
            }

            mapTypes.clear();
            mapTypes = tempTypes;
        }

        m_testListModel->AddTest(aznew VariableLifeCycleTest("Map Variable LifeCycle Test", mapTypes, CreateVariableAction::CreationType::Programmatic));

        AZStd::vector< ScriptCanvas::Data::Type > arrayTypes;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(arrayTypes, &ScriptCanvasEditor::VariableAutomationRequests::GetArrayTypes);

        {
            AZStd::vector< ScriptCanvas::Data::Type > tempTypes;

            for (int i = 0; i < AZStd::GetMin(aznumeric_cast<int>(arrayTypes.size()), 10); ++i)
            {
                tempTypes.emplace_back((*arrayTypes.begin()));
                arrayTypes.erase(arrayTypes.begin());
            }

            arrayTypes.clear();
            arrayTypes = tempTypes;
        }

        m_testListModel->AddTest(aznew VariableLifeCycleTest("Array Variable LifeCycle Test", arrayTypes, CreateVariableAction::CreationType::Programmatic));

        m_testListModel->AddTest(aznew RapidVariableCreationDeletionTest());


        m_testListModel->AddTest(aznew CreateCategoryTest("Logic", nodePaletteWidget));

        m_testListModel->AddTest(aznew CreateGroupTest());
        m_testListModel->AddTest(aznew CreateGroupTest(CreateGroupAction::CreationType::Toolbar));


        m_testListModel->AddTest(aznew GroupManipulationTest(nodePaletteWidget));

        m_testListModel->AddTest(aznew CutCopyPasteDuplicateTest("On Tick"));
        m_testListModel->AddTest(aznew CutCopyPasteDuplicateTest("Multiply (*)"));
        m_testListModel->AddTest(aznew CutCopyPasteDuplicateTest("Print"));
        ////

        ////

        m_tableView->setModel(m_testListModel);
        m_tableView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);

        m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);

        QPushButton* button = new QPushButton();
        button->setText("Run All Tests");
        layout->addWidget(button);

        m_runLabel = new QLabel();
        layout->addWidget(m_runLabel);

        layout->addWidget(m_tableView);

        m_errorTestLabel = new QLabel();
        m_errorTestLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(m_errorTestLabel);

        setLayout(layout);

        setWindowTitle("Editor Automated Testing");

        QObject::connect(button, &QPushButton::clicked, this, &EditorAutomationTestDialog::RunAllTests);

        QObject::connect(m_tableView, &QTableView::doubleClicked, this, &EditorAutomationTestDialog::RunTest);
        QObject::connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &EditorAutomationTestDialog::OnSelectionChanged);
    }

    EditorAutomationTestDialog::~EditorAutomationTestDialog()
    {
        EditorAutomationTestDialogRequestBus::Handler::BusDisconnect();;
    }

    void EditorAutomationTestDialog::RunAllTests()
    {
        for (int i = 0; i < m_testListModel->rowCount(); ++i)
        {
            EditorAutomationTest* test = m_testListModel->FindTestForRow(i);

            if (test)
            {
                m_testQueue.insert(test);
            }
        }

        if (!AZ::SystemTickBus::Handler::BusIsConnected())
        {
            StartNewTestRun();
        }
    }

    void EditorAutomationTestDialog::RunTest(QModelIndex modelIndex)
    {
        EditorAutomationTest* test = m_testListModel->FindTestForIndex(modelIndex);

        if (test)
        {
            m_testQueue.insert(test);

            if (!AZ::SystemTickBus::Handler::BusIsConnected())
            {
                StartNewTestRun();
            }
        }
    }

    void EditorAutomationTestDialog::OnSystemTick()
    {
        auto currentTime = AZStd::chrono::steady_clock::now();
        auto elapsedTimed = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(currentTime - m_startTime);

        if (m_activeTest)
        {
            if (!m_activeTest->IsRunning())
            {
                if (!m_activeTest->HasErrors())
                {
                    ++m_successCount;
                }

                m_testListModel->UpdateTestDisplay(m_activeTest);

                int row = m_testListModel->FindRowForTest(m_activeTest);

                if (m_tableView)
                {
                    if (m_tableView->selectionModel()->currentIndex().row() == row)
                    {
                        DisplayErrorsForTest(m_activeTest);
                    }
                }

                m_activeTest = nullptr;
                m_startTime = AZStd::chrono::steady_clock::now();

                UpdateRunLabel();
            }
        }
        else if (elapsedTimed >= AZStd::chrono::milliseconds(1000))
        {
            if (m_testQueue.empty())
            {
                FinishTestRun();

                // Force the entire layout to refresh for now.
                m_testListModel->layoutChanged();
            }
            else
            {
                m_scriptCanvasWindow->show();
                m_scriptCanvasWindow->raise();
                m_scriptCanvasWindow->activateWindow();
                m_scriptCanvasWindow->setFocus(Qt::FocusReason::MouseFocusReason);

                m_activeTest = (*m_testQueue.begin());
                m_testQueue.erase(m_testQueue.begin());

                m_runCount++;

                m_activeTest->StartTest();

                m_testListModel->UpdateTestDisplay(m_activeTest);
            }
        }
    }

    void EditorAutomationTestDialog::ShowTestDialog()
    {
        show();
        raise();
        activateWindow();
        setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void EditorAutomationTestDialog::OnSelectionChanged(const QItemSelection& selected, const QItemSelection&)
    {
        m_errorTestLabel->clear();

        if (selected.size() == 1)
        {
            EditorAutomationTest* test = m_testListModel->FindTestForIndex(selected.front().indexes().front());

            DisplayErrorsForTest(test);
        }
    }

    void EditorAutomationTestDialog::StartNewTestRun()
    {
        m_runCount = 0;
        m_successCount = 0;

        AZ::SystemTickBus::Handler::BusConnect();
        m_startTime = AZStd::chrono::steady_clock::now();

        UpdateRunLabel();
    }

    void EditorAutomationTestDialog::FinishTestRun()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        m_runLabel->setText(QString("%1 of %2 Test rans successfully").arg(m_successCount).arg(m_runCount));

        ShowTestDialog();
    }

    void EditorAutomationTestDialog::UpdateRunLabel()
    {
        m_runLabel->setText(QString("Running Tests.... %1 remaining.").arg(m_testQueue.size()));
    }

    void EditorAutomationTestDialog::DisplayErrorsForTest(EditorAutomationTest* actionTest)
    {
        AZStd::vector<AZStd::string> errorStrings = actionTest->GetErrors();

        QString displayString;

        for (const AZStd::string& errorString : errorStrings)
        {
            if (!displayString.isEmpty())
            {
                displayString.append("\n");
            }

            displayString.append(errorString.c_str());
        }

        m_errorTestLabel->setText(displayString);
    }

#include <Editor/Source/moc_EditorAutomationTestDialog.cpp>
}
