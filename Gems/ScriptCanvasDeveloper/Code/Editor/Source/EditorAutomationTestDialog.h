/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QAbstractItemModel>
#include <QDialog>
#include <QIcon>
#include <QItemSelection>
#include <QLabel>
#include <QTableView>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationTest.h>

#include <GraphCanvas/Editor/EditorTypes.h>

class QWindow;

namespace ScriptCanvas::Developer
{
    class EditorAutomationTestDialogRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphCanvas::EditorId;

        virtual void ShowTestDialog() = 0;
    };

    using EditorAutomationTestDialogRequestBus = AZ::EBus<EditorAutomationTestDialogRequests>;

    class QtActionTest;

    class TestListModel
        : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TestListModel, AZ::SystemAllocator);

        enum ColumnIndex
        {
            IndexForce = -1,

            TestName,

            Count
        };

        TestListModel();
        ~TestListModel() override;

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent) const override;
        QModelIndex parent(const QModelIndex& index = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        ////

        void AddTest(EditorAutomationTest* actionTest);

        EditorAutomationTest* FindTestForIndex(const QModelIndex& index) const;
        EditorAutomationTest* FindTestForRow(int row) const;

        int FindRowForTest(EditorAutomationTest* actionTest);

        void UpdateTestDisplay(EditorAutomationTest* actionTest);

    private:

        AZStd::vector< EditorAutomationTest* > m_actionTests;

        QIcon m_runningIcon;
        QIcon m_passedIcon;
        QIcon m_failedIcon;
    };

    class EditorAutomationTestDialog
        : public QDialog
        , public AZ::SystemTickBus::Handler
        , public EditorAutomationTestDialogRequestBus::Handler
    {
        Q_OBJECT
    public:
        EditorAutomationTestDialog(QMainWindow* mainWindow);
        ~EditorAutomationTestDialog() override;

        void RunAllTests();
        void RunTest(QModelIndex index);

        // SystemTickBus
        void OnSystemTick() override;
        ////

        // EditorAutomationTestDialogRequestBus::Handler
        void ShowTestDialog() override;
        ////

        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:

        void StartNewTestRun();
        void FinishTestRun();

        void DisplayErrorsForTest(EditorAutomationTest* actionTest);
        void UpdateRunLabel();

        int m_runCount = 0;
        int m_successCount = 0;

        AZStd::chrono::steady_clock::time_point m_startTime;

        EditorAutomationTest* m_activeTest = nullptr;
        AZStd::unordered_set< EditorAutomationTest* > m_testQueue;

        TestListModel* m_testListModel = nullptr;

        QTableView*  m_tableView = nullptr;

        QLabel*      m_errorTestLabel = nullptr;
        QLabel*      m_runLabel = nullptr;
        QMainWindow* m_scriptCanvasWindow = nullptr;
    };
}
