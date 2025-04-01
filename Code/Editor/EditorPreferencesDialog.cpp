/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorPreferencesDialog.h"

// AzQtComponents
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>

#include "EditorPreferencesTreeWidgetItemDelegate.h"

// Editor
#include "MainWindow.h"
#include "EditorPreferencesTreeWidgetItem.h"
#include "DisplaySettings.h"
#include "CryEditDoc.h"
#include "Controls/ConsoleSCB.h"
#include "EditorPreferencesPageGeneral.h"
#include "EditorPreferencesPageFiles.h"
#include "EditorPreferencesPageViewportGeneral.h"
#include "EditorPreferencesPageViewportManipulator.h"
#include "EditorPreferencesPageViewportCamera.h"
#include "EditorPreferencesPageViewportDebug.h"
#include "EditorPreferencesPageAWS.h"
#include "LyViewPaneNames.h"
#include "Entity/EditorEntityHelpers.h"

// Editor
#include "EditorPreferencesPageGeneral.h"
#include "EditorPreferencesPageFiles.h"
#include "EditorPreferencesPageViewportGeneral.h"
#include "EditorPreferencesPageViewportManipulator.h"
#include "EditorPreferencesPageViewportCamera.h"
#include "EditorPreferencesPageViewportDebug.h"
#include "EditorPreferencesPageAWS.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_EditorPreferencesDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

using namespace AzQtComponents;

EditorPreferencesDialog::EditorPreferencesDialog(QWidget* pParent)
    : QDialog(new WindowDecorationWrapper(WindowDecorationWrapper::OptionAutoAttach | WindowDecorationWrapper::OptionAutoTitleBarButtons, pParent))
    , ui(new Ui::EditorPreferencesDialog)
    , m_currentPageItem(nullptr)
{
    ui->setupUi(this);

    ui->filter->SetTypeFilterVisible(false);
    connect(ui->filter, &FilteredSearchWidget::TextFilterChanged, this, &EditorPreferencesDialog::SetFilter);

    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(serializeContext, "Serialization context not available");

    static bool bAlreadyRegistered = false;
    if (!bAlreadyRegistered)
    {
        bAlreadyRegistered = true;
        if (serializeContext)
        {
            CEditorPreferencesPage_General::Reflect(*serializeContext);
            CEditorPreferencesPage_Files::Reflect(*serializeContext);
            CEditorPreferencesPage_ViewportGeneral::Reflect(*serializeContext);
            CEditorPreferencesPage_ViewportManipulator::Reflect(*serializeContext);
            CEditorPreferencesPage_ViewportCamera::Reflect(*serializeContext);
            CEditorPreferencesPage_ViewportDebug::Reflect(*serializeContext);
            CEditorPreferencesPage_AWS::Reflect(*serializeContext);
        }
    }

    ui->propertyEditor->SetAutoResizeLabels(true);
    ui->propertyEditor->Setup(serializeContext, this, true, 250);

    ui->pageTree->setColumnCount(1);

    // There are no expandable categories, so hide the column.
    ui->pageTree->setRootIsDecorated(false);

    // Set the delegate so we can use the svg icons.
    ui->pageTree->setItemDelegate(new EditorPreferencesTreeWidgetItemDelegate(ui->pageTree));

    // Shrink the spacer so that the search bar fills the dialog.
    ui->horizontalSpacer_2->changeSize(0, 0, QSizePolicy::Maximum);

    connect(ui->pageTree, &QTreeWidget::currentItemChanged, this, &EditorPreferencesDialog::OnTreeCurrentItemChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &EditorPreferencesDialog::OnAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &EditorPreferencesDialog::OnReject);
    connect(ui->MANAGE_BTN, &QPushButton::clicked, this, &EditorPreferencesDialog::OnManage);

    AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("style:EditorPreferencesDialog.qss"));
}

EditorPreferencesDialog::~EditorPreferencesDialog()
{
}

void EditorPreferencesDialog::SetFilterText(const QString& filter)
{
    ui->filter->SetTextFilter(filter);
}

void EditorPreferencesDialog::showEvent(QShowEvent* event)
{
    origAutoBackup.bEnabled = gSettings.autoBackupEnabled;
    origAutoBackup.nTime = gSettings.autoBackupTime;
    origAutoBackup.nRemindTime = gSettings.autoRemindTime;

    CreateImages();
    CreatePages();
    ui->pageTree->setCurrentItem(ui->pageTree->topLevelItem(0));
    QDialog::showEvent(event);
}

bool WidgetConsumesKeyPressEvent(QKeyEvent* event)
{
    // If the enter key is pressed during any text input, the dialog box will close
    // making it inconvenient to do multiple edits. This routine captures the
    // Key_Enter or Key_Return and clears the focus to give a visible cue that
    // editing of that field has finished and then doesn't propagate it.
    if (event->key() != Qt::Key::Key_Enter && event->key() != Qt::Key::Key_Return)
    {
        return false;
    }

    if (QWidget* editWidget = QApplication::focusWidget())
    {
        editWidget->clearFocus();
    }

    return true;
}

void EditorPreferencesDialog::keyPressEvent(QKeyEvent* event)
{
    if (!WidgetConsumesKeyPressEvent(event))
    {
        QDialog::keyPressEvent(event);
    }
}

void EditorPreferencesDialog::OnTreeCurrentItemChanged()
{
    QTreeWidgetItem* currentItem = ui->pageTree->currentItem();

    if (currentItem->type() == EditorPreferencesTreeWidgetItem::EditorPreferencesPage)
    {
        EditorPreferencesTreeWidgetItem* currentPageItem = static_cast<EditorPreferencesTreeWidgetItem*>(currentItem);
        if (currentPageItem != m_currentPageItem)
        {
            SetActivePage(currentPageItem);
        }
    }
    else
    {
        if (m_currentPageItem == nullptr || m_currentPageItem->parent() != currentItem)
        {
            EditorPreferencesTreeWidgetItem* child = (EditorPreferencesTreeWidgetItem*)currentItem->child(0);
            SetActivePage(child);
        }
    }
}

void EditorPreferencesDialog::OnAccept()
{
    // Call on OK for all pages.
    QTreeWidgetItemIterator it(ui->pageTree);
    while (*it)
    {
        QTreeWidgetItem* item = *it;
        if (item->type() == EditorPreferencesTreeWidgetItem::EditorPreferencesPage)
        {
            EditorPreferencesTreeWidgetItem* pageItem = (EditorPreferencesTreeWidgetItem*)*it;
            pageItem->GetPreferencesPage()->OnApply();
        }
        ++it;
    }

    // Save settings.
    gSettings.Save();
    GetIEditor()->GetDisplaySettings()->SaveRegistry();

    if (GetIEditor()->GetDocument()->IsDocumentReady() && (
            origAutoBackup.bEnabled != gSettings.autoBackupEnabled ||
            origAutoBackup.nTime != gSettings.autoBackupTime ||
            origAutoBackup.nRemindTime != gSettings.autoRemindTime))
    {
        // Ensure timers restart with the correct interval.
        MainWindow::instance()->ResetAutoSaveTimers();
    }

    AzToolsFramework::EditorPreferencesNotificationBus::Broadcast(&AzToolsFramework::EditorPreferencesNotifications::OnEditorPreferencesChanged);

    accept();
}

void EditorPreferencesDialog::OnReject()
{
    // QueryCancel for all pages.
    QTreeWidgetItemIterator it(ui->pageTree);
    while (*it)
    {
        QTreeWidgetItem* item = *it;
        if (item->type() == EditorPreferencesTreeWidgetItem::EditorPreferencesPage)
        {
            EditorPreferencesTreeWidgetItem* pageItem = (EditorPreferencesTreeWidgetItem*)*it;
            if (!pageItem->GetPreferencesPage()->OnQueryCancel())
            {
                return;
            }
        }
        ++it;
    }

    QTreeWidgetItemIterator cancelIt(ui->pageTree);
    while (*cancelIt)
    {
        QTreeWidgetItem* item = *cancelIt;
        if (item->type() == EditorPreferencesTreeWidgetItem::EditorPreferencesPage)
        {
            EditorPreferencesTreeWidgetItem* pageItem = (EditorPreferencesTreeWidgetItem*)*cancelIt;
            pageItem->GetPreferencesPage()->OnCancel();
        }
        ++cancelIt;
    }


    reject();
}

void EditorPreferencesDialog::OnManage()
{
    GetIEditor()->OpenView(LyViewPane::EditorSettingsManager);
    OnAccept();
}

void EditorPreferencesDialog::SetActivePage(EditorPreferencesTreeWidgetItem* pageItem)
{
    m_currentPageItem = pageItem;

    ui->propertyEditor->ClearInstances();
    IPreferencesPage* instance = m_currentPageItem->GetPreferencesPage();
    const AZ::Uuid& classId = AZ::SerializeTypeInfo<IPreferencesPage>::GetUuid(instance);
    ui->propertyEditor->AddInstance(instance, classId);
    m_currentPageItem->UpdateEditorFilter(ui->propertyEditor, m_filter);

    ui->propertyEditor->show();

    // Refresh the Stylesheet - style would break on page load sometimes.
    AzQtComponents::StyleManager::repolishStyleSheet(this);
}

void EditorPreferencesDialog::SetFilter(const QString& filter)
{
    m_filter = filter;

    QTreeWidgetItem* firstVisiblePage = nullptr;

    std::function<void(QTreeWidgetItem* item)> filterItem = [&](QTreeWidgetItem* item)
    {
        // Hide categories that have no pages remaining in them after filtering their pages
        if (item->childCount() > 0)
        {
            bool shouldHide = true;
            for (int i = item->childCount() - 1; i >= 0; --i)
            {
                QTreeWidgetItem* child = item->child(i);
                filterItem(child);
                shouldHide = shouldHide && child->isHidden();
            }
            item->setHidden(shouldHide);
        }
        else
        {
            EditorPreferencesTreeWidgetItem* pageItem = static_cast<EditorPreferencesTreeWidgetItem*>(item);
            pageItem->Filter(filter);
            if (!firstVisiblePage && !pageItem->isHidden())
            {
                firstVisiblePage = pageItem;
            }
        }
    };
    filterItem(ui->pageTree->invisibleRootItem());

    // If we're searching and we don't have a current selection any more (the old page got filtered)
    // go ahead and select the first valid page
    if ((!m_currentPageItem || m_currentPageItem->isHidden())
        && !filter.isEmpty() && firstVisiblePage)
    {
        ui->pageTree->setCurrentItem(firstVisiblePage);
    }
    else if (m_currentPageItem)
    {
        m_currentPageItem->UpdateEditorFilter(ui->propertyEditor, m_filter);

        // Refresh the Stylesheet - when using search functionality.
        AzQtComponents::StyleManager::repolishStyleSheet(this);
    }
}

void EditorPreferencesDialog::CreateImages()
{
    m_selectedPixmap = QPixmap(":/res/Preferences_00.png");
    m_unSelectedPixmap = QPixmap(":/res/Preferences_01.png");
}

void EditorPreferencesDialog::CreatePages()
{

    auto addPreferencePage = [&](IPreferencesPage* page) {
       ui->pageTree->addTopLevelItem(new EditorPreferencesTreeWidgetItem(page, page->GetIcon()));
    };
    addPreferencePage(new CEditorPreferencesPage_General());
    addPreferencePage(new CEditorPreferencesPage_Files());
    addPreferencePage(new CEditorPreferencesPage_ViewportGeneral());
    addPreferencePage(new CEditorPreferencesPage_ViewportCamera());
    addPreferencePage(new CEditorPreferencesPage_ViewportManipulator());
    addPreferencePage(new CEditorPreferencesPage_ViewportDebug());
    if (AzToolsFramework::IsComponentWithServiceRegistered(AZ_CRC_CE("AWSCoreEditorService")))
    {
        addPreferencePage(new CEditorPreferencesPage_AWS());
    }
}

void EditorPreferencesDialog::AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
{
    // ensure we refresh all the property editor values as it is possible for them to
    // change based on other values (e.g. legacy ui and new viewport not being compatible)
    ui->propertyEditor->InvalidateValues();
}
