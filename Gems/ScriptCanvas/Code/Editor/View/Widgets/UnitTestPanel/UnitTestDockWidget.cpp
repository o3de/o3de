/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include <AzCore/IO/FileIO.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#include <Editor/QtMetaTypes.h>
#include <Editor/Settings.h>

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/Model/UnitTestBrowserFilterModel.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/PropertyGridBus.h>
#include <Editor/View/Widgets/UnitTestPanel/UnitTestDockWidget.h>
#include <Editor/View/Widgets/UnitTestPanel/ui_UnitTestDockWidget.h>
#include <Editor/View/Widgets/UnitTestPanel/moc_UnitTestDockWidget.cpp>

#include <Data/Data.h>

#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/ScriptCanvasExecutionBus.h>
#include <ScriptCanvas/Bus/UnitTestVerificationBus.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <LyViewPaneNames.h>

namespace ScriptCanvasEditor
{

    /////////////////////////
    // ItemButtonsDelegate
    /////////////////////////

    ItemButtonsDelegate::ItemButtonsDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
        , m_editIcon(QIcon(":/ScriptCanvasEditorResources/Resources/edit_icon.png").pixmap(QSize(14, 14)))
    {
    }

    QPoint ItemButtonsDelegate::GetEditPosition(const QStyleOptionViewItem& option) const
    {
        return QPoint(option.rect.right() - m_editIcon.width(), option.rect.center().y() - m_editIcon.height() / 2);
    }

    QPoint ItemButtonsDelegate::GetResultsPosition(const QStyleOptionViewItem& option) const
    {
        return QPoint(option.rect.left() + m_editIcon.width() + m_leftIconPadding, option.rect.center().y() - m_editIcon.height() / 2);
    }

    void ItemButtonsDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyledItemDelegate::paint(painter, option, index);

        if (!index.model()->index(0, 0, index).isValid() && (option.state & QStyle::State_MouseOver))
        {
            painter->drawPixmap(GetEditPosition(option), m_editIcon);
        }
    }

    bool ItemButtonsDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        if (!index.model()->index(0, 0, index).isValid() && event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            QRect editButtonRect = m_editIcon.rect().translated(GetEditPosition(option));
            QRect resultsButtonRect = m_editIcon.rect().translated(GetResultsPosition(option));

            if (editButtonRect.contains(mouseEvent->pos()))
            {
                Q_EMIT EditButtonClicked(index);
            }
            else if (resultsButtonRect.contains(mouseEvent->pos()))
            {
                Q_EMIT ResultsButtonClicked(index);
            }
        }

        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    ///////////////////////
    // UnitTestComponent
    ///////////////////////

    void UnitTestComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<UnitTestComponent, GraphCanvas::GraphCanvasPropertyComponent>()
                ->Version(0)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<UnitTestComponent>("Unit Test", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &UnitTestComponent::GetTitle)
                    ;
            }
        }
    }

    AZ::Entity* UnitTestComponent::CreateUnitTestEntity()
    {
        AZ::Entity* entity = aznew AZ::Entity("UnitTestHelper");

        entity->CreateComponent<UnitTestComponent>();

        return entity;
    }

    UnitTestComponent::UnitTestComponent()
        : m_componentTitle("UnitTest")
    {
    }

    AZStd::string_view UnitTestComponent::GetTitle()
    {
        return m_componentTitle;
    }

    /////////////////////////
    // UnitTestContextMenu
    /////////////////////////

    UnitTestContextMenu::UnitTestContextMenu(UnitTestDockWidget* dockWidget, AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* sourceEntry)
        : QMenu()
    {
        AZ::Uuid sourceUuid = sourceEntry->GetSourceUuid();
        AZStd::string sourceDisplayName = sourceEntry->GetDisplayName().toUtf8().data();

        if (dockWidget->widgetActive)
        {
            QAction* runAction = new QAction(QObject::tr("Run this test"), this);
            runAction->setToolTip(QObject::tr("Run this Test only."));
            runAction->setStatusTip(QObject::tr("Run this Test only."));

            QObject::connect(runAction,
                &QAction::triggered,
                [dockWidget, sourceUuid]()
                {
                    AZStd::vector<AZ::Uuid> scriptUuids;
                    scriptUuids.push_back(sourceUuid);

                    dockWidget->RunTests(scriptUuids);
                }
            );
            addAction(runAction);

            if (dockWidget->m_filter->HasTestResults(sourceUuid))
            {
                QAction* consoleAction = new QAction(QObject::tr("View test results"), this);
                consoleAction->setToolTip(QObject::tr("Read Console Results for this Test."));
                consoleAction->setStatusTip(QObject::tr("Read Console Results for this Test."));

                QObject::connect(consoleAction,
                    &QAction::triggered,
                    [dockWidget, sourceUuid, sourceDisplayName]()
                    {
                        dockWidget->OpenTestResults(sourceUuid, sourceDisplayName);
                    }
                );
                addAction(consoleAction);
            }
        }

        QAction* openAction = new QAction(QObject::tr("Edit script"), this);
        openAction->setToolTip(QObject::tr("Open this Test in the Script Canvas Editor."));
        openAction->setStatusTip(QObject::tr("Open this Test in the Script Canvas Editor."));

        QObject::connect(openAction,
            &QAction::triggered,
            [dockWidget, sourceUuid]()
            {
                dockWidget->OpenScriptInEditor(sourceUuid);
            }
        );
        addAction(openAction);
    }

    ////////////////////////
    // UnitTestDockWidget
    ////////////////////////

    UnitTestDockWidget::UnitTestDockWidget(QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDockWidget(parent)
        , m_ui(new Ui::UnitTestDockWidget())
        , widgetActive(true)
        , m_itemButtonsDelegate(new ItemButtonsDelegate(this))
    {
        m_ui->setupUi(this);

        UnitTestWidgetNotificationBus::Handler::BusConnect();

        m_ui->searchFilter->setClearButtonEnabled(true);
        QObject::connect(m_ui->searchFilter, &QLineEdit::textChanged, this, &UnitTestDockWidget::OnQuickFilterChanged);
        QObject::connect(m_ui->searchFilter, &QLineEdit::returnPressed, this, &UnitTestDockWidget::OnReturnPressed);

        m_filterTimer.setInterval(250);
        m_filterTimer.setSingleShot(true);
        m_filterTimer.stop();

        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &UnitTestDockWidget::UpdateSearchFilter);

        m_ui->testsTree->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(m_ui->testsTree, &QWidget::customContextMenuRequested, this, &UnitTestDockWidget::OnContextMenuRequested);
        connect(m_ui->closeResults, &QPushButton::clicked, this, &UnitTestDockWidget::OnCloseResultsButton);

        m_filter = m_ui->testsTree->m_filter;

        m_ui->testsTree->setItemDelegateForColumn(0, m_itemButtonsDelegate);
        QObject::connect(m_itemButtonsDelegate, &ItemButtonsDelegate::EditButtonClicked, this, &UnitTestDockWidget::OnEditButtonClicked);
        QObject::connect(m_itemButtonsDelegate, &ItemButtonsDelegate::ResultsButtonClicked, this, &UnitTestDockWidget::OnResultsButtonClicked);

        if (UnitTestVerificationBus::GetTotalNumOfEventHandlers() == 0)
        {
            m_ui->testResultsOutput->setPlainText(QString("WARNING: Functionality of this Widget has been limited - Script Canvas Testing Gem is not loaded!"));
            m_ui->runButton->setDisabled(true);
            widgetActive = false;
        }
        else
        {
            m_ui->consoleOutput->hide();
            connect(m_ui->runButton, &QPushButton::clicked, this, &UnitTestDockWidget::OnStartTestsButton);
            connect(m_ui->testsTree, &QAbstractItemView::doubleClicked, this, &UnitTestDockWidget::OnRowDoubleClicked);
        }
    }

    UnitTestDockWidget::~UnitTestDockWidget()
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        UnitTestWidgetNotificationBus::Handler::BusDisconnect();
        delete m_itemButtonsDelegate;
    }

    void UnitTestDockWidget::OnCheckStateCountChange(const int count)
    {
        m_ui->label->setText(QString("Selected %1 test(s).").arg(count));
    }

    void UnitTestDockWidget::OnContextMenuRequested(const QPoint& pos)
    {
        QModelIndex index = m_ui->testsTree->indexAt(pos);
        QModelIndex sourceIndex = m_filter->mapToSource(index);

        if (sourceIndex.isValid())
        {
            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source)
            {
                UnitTestContextMenu menu(this, static_cast<AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry));
                menu.exec(m_ui->testsTree->viewport()->mapToGlobal(pos));
            }
        }
    }

    void UnitTestDockWidget::OnRowDoubleClicked(QModelIndex index)
    {
        QModelIndex sourceIndex = m_filter->mapToSource(index);

        if (sourceIndex.isValid())
        {
            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source)
            {
                AZStd::vector<AZ::Uuid> scriptUuids;
                scriptUuids.emplace_back(static_cast<AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry)->GetSourceUuid());

                RunTests(scriptUuids);
            }
        }
    }

    void UnitTestDockWidget::OnEditButtonClicked(QModelIndex index)
    {
        QModelIndex sourceIndex = m_filter->mapToSource(index);

        if (sourceIndex.isValid())
        {
            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source)
            {
                AZ::Uuid sourceUuid = static_cast<AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry)->GetSourceUuid();

                OpenScriptInEditor(sourceUuid);
            }
        }
    }

    void UnitTestDockWidget::OnResultsButtonClicked(QModelIndex index)
    {
        QModelIndex sourceIndex = m_filter->mapToSource(index);

        if (sourceIndex.isValid())
        {
            AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(sourceIndex.internalPointer());

            if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source)
            {
                AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* sourceEntry = static_cast<AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry);
                AZ::Uuid sourceUuid = sourceEntry->GetSourceUuid();
                AZStd::string sourceDisplayName = sourceEntry->GetDisplayName().toUtf8().data();

                OpenTestResults(sourceUuid, sourceDisplayName);
            }
        }
    }

    QCheckBox* UnitTestDockWidget::GetEnabledCheckBox(ScriptCanvas::ExecutionMode mode)
    {
        switch (mode)
        {
        case ScriptCanvas::ExecutionMode::Interpreted:
            return m_ui->executionInterpretedEnabled;

        case ScriptCanvas::ExecutionMode::Native:
            return m_ui->executionNativeEnabled;

        default:
            AZ_Assert(false, "Unsupported type");
            return nullptr;
        }
    }
    
    QLabel* UnitTestDockWidget::GetStatusLabel(ScriptCanvas::ExecutionMode mode)
    {
         switch (mode)
         {
         case ScriptCanvas::ExecutionMode::Interpreted:
             return m_ui->labelInterpretedStatus;
         
         case ScriptCanvas::ExecutionMode::Native:
             return m_ui->labelNativeStatus;
         
         default:
            AZ_Assert(false, "Unsupported type");
            return nullptr;
         }
    }

    void UnitTestDockWidget::ClearSearchFilter()
    {
        {
            QSignalBlocker blocker(m_ui->searchFilter);
            m_ui->searchFilter->setText("");
        }

        UpdateSearchFilter();
    }

    void UnitTestDockWidget::UpdateSearchFilter()
    {
        m_ui->testsTree->SetSearchFilter(m_ui->searchFilter->userInputText());
    }

    void UnitTestDockWidget::OnReturnPressed()
    {
        UpdateSearchFilter();
    }

    void UnitTestDockWidget::OnQuickFilterChanged(const QString& text)
    {
        if(text.isEmpty())
        {
            //If filter was cleared, update immediately
            UpdateSearchFilter();
            return;
        }

        m_filterTimer.stop();
        m_filterTimer.start();
    }

    void UnitTestDockWidget::OnStartTestsButton()
    {
        AZStd::vector<AZ::Uuid> scriptUuids;
        m_filter->GetCheckedScriptsUuidsList(scriptUuids);

        ClearSearchFilter();

        RunTests(scriptUuids);
    }

    void UnitTestDockWidget::OnCloseResultsButton()
    {
        m_ui->consoleOutput->hide();
    }

    void UnitTestDockWidget::OpenScriptInEditor(AZ::Uuid sourceUuid)
    {
        AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);
        AZ::Data::AssetId sourceAssetId(sourceUuid, 0);

        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
        GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAssetId
            , ScriptCanvasEditor::SourceHandle(nullptr, sourceUuid, {})
            , Tracker::ScriptCanvasFileState::UNMODIFIED);

        if (!openOutcome)
        {
            AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
        }
    }

    void UnitTestDockWidget::OpenTestResults(AZ::Uuid sourceUuid, AZStd::string_view sourceDisplayName)
    {
        if (m_filter->HasTestResults(sourceUuid))
        {
            m_ui->testResultsLabel->setText(QString("Test Results | %1").arg(sourceDisplayName.data()));
            m_ui->testResultsOutput->setPlainText(QString(m_filter->GetTestResult(sourceUuid)->m_consoleOutput.c_str()));
            m_ui->consoleOutput->show();
        }
    }

    QString ModeToString(ScriptCanvas::ExecutionMode mode)
    {
        using namespace ScriptCanvas;

        switch (mode)
        {
        case ExecutionMode::Interpreted:
            return QString("Interpreted");
        
        case ExecutionMode::Native:
            return QString("Native");
        
        default:
            return QString("<invalid>");
        }
    }
    
    bool UnitTestDockWidget::IsModeEnabled(ScriptCanvas::ExecutionMode mode)
    {
        return GetEnabledCheckBox(mode)->checkState() == Qt::Checked;
    }

    void UnitTestDockWidget::RunTests(const AZStd::vector<AZ::Uuid>& scriptUuids)
    {
        AZStd::vector<ScriptCanvas::ExecutionMode> activeModes;

        auto executionModes = { ExecutionMode::Interpreted, ExecutionMode::Native };
        for (auto mode : executionModes)
        {
            if (IsModeEnabled(mode))
            {
                activeModes.push_back(mode);
            }
            else
            {
                GetStatusLabel(mode)->setText(ModeToString(mode) + QString(" not running"));
            }
        }
        
        if (activeModes.empty() || scriptUuids.empty())
        {
            m_ui->consoleOutput->hide();
            m_filter->FlushLatestTestRun();
            return;
        }
        else
        {
            AZ::SystemTickBus::Handler::BusConnect();

            m_ui->label->setText(QString("Starting %1 tests.").arg(scriptUuids.size()));
            m_filter->FlushLatestTestRun();
            m_filter->TestsStart();
            m_ui->consoleOutput->hide();

            for (size_t modeIndex = 0; modeIndex < activeModes.size(); ++modeIndex)
            {
                auto mode = activeModes[modeIndex];

                GetStatusLabel(mode)->setText(QString("Starting %1 tests.").arg(scriptUuids.size()));
                
                for (const AZ::Uuid& scriptUuid : scriptUuids)
                {                    
                    const SourceAssetBrowserEntry* sourceBrowserEntry = SourceAssetBrowserEntry::GetSourceByUuid(scriptUuid);
                    if (sourceBrowserEntry == nullptr)
                    {
                        AZ_Error("Script Canvas", false, "The source asset file with ID: %s was not found", scriptUuid.ToString<AZStd::string>().c_str());
                        continue;
                    }

                    AZ::Data::AssetInfo assetInfo;
                    if (AssetHelpers::GetAssetInfo(sourceBrowserEntry->GetFullPath(), assetInfo))
                    {
                        auto asset = AZ::Data::AssetManager::Instance().GetAsset(assetInfo.m_assetId, azrtti_typeid<ScriptCanvasAsset>(), AZ::Data::AssetLoadBehavior::PreLoad);
                        asset.BlockUntilLoadComplete();
                        if (asset.IsReady())
                        {
                            RunTestGraph(asset, mode);
                        }
                    }
                }
            }           
        }
    }

    void UnitTestDockWidget::OnTestsComplete()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        QString testCompletionString;

        const int nativeMode = static_cast<int>(ExecutionMode::Native);
        if (m_testMetrics[nativeMode].m_graphsTested > 0)
        {
            testCompletionString = ModeToString(ExecutionMode::Native);
            testCompletionString += QString(": ");

            testCompletionString += QString("Attempted %1 test(s) - %2 Succeeded, %3 Failed, %4 Failed to Compile")
                .arg(m_testMetrics[nativeMode].m_graphsTested)
                .arg(m_testMetrics[nativeMode].m_success)
                .arg(m_testMetrics[nativeMode].m_failures)
                .arg(m_testMetrics[nativeMode].m_compilationFailures);

            GetStatusLabel(ExecutionMode::Native)->setText(testCompletionString);
        }

        const int interpretedMode = static_cast<int>(ExecutionMode::Interpreted);
        if (m_testMetrics[interpretedMode].m_graphsTested > 0)
        {
            testCompletionString = ModeToString(ExecutionMode::Interpreted);
            testCompletionString += QString(": ");

            testCompletionString += QString("Attempted %1 test(s) - %2 Succeeded, %3 Failed, %4 Failed to Compile")
                .arg(m_testMetrics[interpretedMode].m_graphsTested)
                .arg(m_testMetrics[interpretedMode].m_success)
                .arg(m_testMetrics[interpretedMode].m_failures)
                .arg(m_testMetrics[interpretedMode].m_compilationFailures);

            GetStatusLabel(ExecutionMode::Interpreted)->setText(testCompletionString);
        }

        m_filter->TestsEnd();
        m_ui->label->setText(QString("Finished"));

        m_testMetrics[nativeMode].Clear();
        m_testMetrics[interpretedMode].Clear();
    }

    void UnitTestDockWidget::RunTestGraph(AZ::Data::Asset<AZ::Data::AssetData> asset, ScriptCanvas::ExecutionMode mode)
    {
        Reporter reporter;
        UnitTestWidgetNotificationBus::Broadcast(&UnitTestWidgetNotifications::OnTestStart, asset.GetId().m_guid);

        ScriptCanvasExecutionBus::BroadcastResult(reporter, &ScriptCanvasExecutionRequests::RunAssetGraph, asset, mode);

        UnitTestResult testResult;

        UnitTestVerificationBus::BroadcastResult(testResult, &UnitTestVerificationRequests::Verify, reporter);
        UnitTestWidgetNotificationBus::Broadcast(&UnitTestWidgetNotifications::OnTestResult, asset.GetId().m_guid, testResult);

        m_pendingTests.Add(asset.GetId(), mode);

        ++m_testMetrics[static_cast<int>(mode)].m_graphsTested;

        if (testResult.m_compiled)
        {
            if (testResult.m_completed)
            {
                ++m_testMetrics[static_cast<int>(mode)].m_success;
            }
            else
            {
                ++m_testMetrics[static_cast<int>(mode)].m_failures;
            }
        }
        else
        {
            ++m_testMetrics[static_cast<int>(mode)].m_compilationFailures;
        }

        m_pendingTests.Complete(asset.GetId(), mode);
    }

    void UnitTestDockWidget::OnSystemTick()
    {
        if (m_pendingTests.IsFinished())
        {
            OnTestsComplete();
        }
    }

    void UnitTestDockWidget::PendingTests::Add(AZ::Data::AssetId assetId, ExecutionMode mode)
    {
        m_pendingTests.push_back(AZStd::make_pair(assetId, mode));
    }

    void UnitTestDockWidget::PendingTests::Complete(AZ::Data::AssetId assetId, ExecutionMode mode)
    {
        AZStd::erase_if(m_pendingTests, [assetId, mode](const AZStd::pair<AZ::Data::AssetId, ExecutionMode>& pending)
            {
                return (assetId == pending.first && mode == pending.second);
            });
    }

    bool UnitTestDockWidget::PendingTests::IsFinished() const
    {
        return m_pendingTests.empty();
    }

}
