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
#include <precompiled.h>

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

#include <Data/Data.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/QtMetaTypes.h>
#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/PropertyGridBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/Model/UnitTestBrowserFilterModel.h>

#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <ScriptCanvas/Bus/ScriptCanvasExecutionBus.h>
#include <ScriptCanvas/Bus/UnitTestVerificationBus.h>

#include <LyViewPaneNames.h>

#include <Editor/View/Widgets/UnitTestPanel/UnitTestDockWidget.h>
#include <Editor/View/Widgets/UnitTestPanel/ui_UnitTestDockWidget.h>
#include <Editor/View/Widgets/UnitTestPanel/moc_UnitTestDockWidget.cpp>


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
        GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAssetId, sourceAssetId);
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

    void UnitTestDockWidget::RunTests(const AZStd::vector<AZ::Uuid>& scriptUuids)
    {
        m_ui->consoleOutput->hide();

        m_filter->FlushLatestTestRun();
        m_filter->TestsStart();

        int successCount = 0;
        int failureCount = 0;

        m_ui->label->setText(QString("Starting %1 tests.").arg(scriptUuids.size()));
        
        for (const AZ::Uuid& scriptUuid : scriptUuids)
        {
            const SourceAssetBrowserEntry* sourceBrowserEntry = SourceAssetBrowserEntry::GetSourceByUuid(scriptUuid);

            if (sourceBrowserEntry == nullptr)
            {
                ++failureCount;
            }
            else
            {
                AZStd::string scriptAbsolutePath = sourceBrowserEntry->GetFullPath();

                Reporter reporter;

                UnitTestWidgetNotificationBus::Broadcast(&UnitTestWidgetNotifications::OnTestStart, scriptUuid);

                ScriptCanvasExecutionBus::BroadcastResult(reporter, &ScriptCanvasExecutionRequests::RunGraph, scriptAbsolutePath);

                UnitTestResult testResult;
                UnitTestVerificationBus::BroadcastResult(testResult, &UnitTestVerificationRequests::Verify, reporter);

                UnitTestWidgetNotificationBus::Broadcast(&UnitTestWidgetNotifications::OnTestResult, scriptUuid, testResult);

                if (testResult.m_success)
                {
                    ++successCount;
                }
                else
                {
                    ++failureCount;
                }
            }
            
            QString executionMessage = QString("Executed %1 out of %2 test(s) - ").arg(successCount + failureCount).arg(scriptUuids.size());

            if (successCount > 0)
            {
                executionMessage += QString("%1 passed").arg(successCount);
            }

            if (successCount > 0 && failureCount > 0)
            {
                executionMessage += ", ";
            }

            if (failureCount > 0)
            {
                executionMessage += QString("%1 failed").arg(failureCount);
            }

            executionMessage += ".";

            m_ui->label->setText(QString(executionMessage));
        }

        m_filter->TestsEnd();
    }
}
