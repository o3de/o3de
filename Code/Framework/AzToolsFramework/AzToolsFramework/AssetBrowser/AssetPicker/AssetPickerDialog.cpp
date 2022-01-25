/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UserSettings/UserSettings.h>

#include <AzQtComponents/Components/DockBar.h>
#include <AzCore/Console/IConsole.h>

#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzToolsFramework/AssetBrowser/AssetPicker/AssetPickerDialog.h>

AZ_PUSH_DISABLE_WARNING(4251 4244, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <AzToolsFramework/AssetBrowser/AssetPicker/ui_AssetPickerDialog.h>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QTimer>
AZ_POP_DISABLE_WARNING

AZ_CVAR(
    bool, ed_hideAssetPickerPathColumn, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Hide AssetPicker path column for a clearer view.");

AZ_CVAR(
    bool, ed_useNewAssetPickerView, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Uses the new Asset Picker View.");

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetPickerDialog::AssetPickerDialog(AssetSelectionModel& selection, QWidget* parent)
            : QDialog(parent)
            , m_ui(new Ui::AssetPickerDialogClass())
            , m_filterModel(new AssetBrowserFilterModel(parent))
            , m_tableModel(new AssetBrowserTableModel(parent))
            , m_selection(selection)
            , m_hasFilter(false)
        {
            m_filterStateSaver = AzToolsFramework::TreeViewState::CreateTreeViewState();

            m_ui->setupUi(this);
            m_ui->m_searchWidget->Setup(true, false);

            m_ui->m_searchWidget->GetFilter()->AddFilter(m_selection.GetDisplayFilter());

            using namespace AzToolsFramework::AssetBrowser;
            AssetBrowserComponentRequestBus::BroadcastResult(m_assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
            AZ_Assert(m_assetBrowserModel, "Failed to get asset browser model");
            m_filterModel->setSourceModel(m_assetBrowserModel);
            m_filterModel->SetFilter(m_ui->m_searchWidget->GetFilter());

            QString name = m_selection.GetDisplayFilter()->GetName();

            m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel.data());
            m_ui->m_assetBrowserTreeViewWidget->setSelectionMode(selection.GetMultiselect() ?
                QAbstractItemView::SelectionMode::ExtendedSelection : QAbstractItemView::SelectionMode::SingleSelection);
            m_ui->m_assetBrowserTreeViewWidget->setDragEnabled(false);

            // if the current selection is invalid, disable the Ok button
            m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(EvaluateSelection());
            m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setProperty("class", "Primary");
            m_ui->m_buttonBox->button(QDialogButtonBox::Cancel)->setProperty("class", "Secondary");

            connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel.data(), &AssetBrowserFilterModel::filterUpdatedSlot);
            connect(m_filterModel.data(), &AssetBrowserFilterModel::filterChanged, this, [this]()
            {
                const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
                const bool selectFirstFilteredIndex = true;
                m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
            });
            connect(m_ui->m_assetBrowserTreeViewWidget, &QAbstractItemView::doubleClicked, this, &AssetPickerDialog::DoubleClickedSlot);
            connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal, this,
                [this](const QItemSelection&, const QItemSelection&){ AssetPickerDialog::SelectionChangedSlot(); });
            connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
            connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

            m_ui->m_assetBrowserTreeViewWidget->SetName("AssetBrowserTreeView_" + name);

            bool selectedAsset = false;

            for (auto& assetId : selection.GetSelectedAssetIds())
            {
                if (assetId.IsValid())
                {
                    selectedAsset = true;
                    m_ui->m_assetBrowserTreeViewWidget->SelectProduct(assetId);
                }
            }

            if (selection.GetSelectedAssetIds().empty())
            {
                for (auto& filePath : selection.GetSelectedFilePaths())
                {
                    if (!filePath.empty())
                    {
                        selectedAsset = true;
                        m_ui->m_assetBrowserTreeViewWidget->SelectFileAtPath(filePath);
                    }
                }
            }

            if (!selectedAsset)
            {
                m_ui->m_assetBrowserTreeViewWidget->SelectFolder(selection.GetDefaultDirectory());
            }

            setWindowTitle(tr("Pick %1").arg(m_selection.GetTitle()));

            m_persistentState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(AZ::Crc32(("AssetBrowserTreeView_Dialog_" + name).toUtf8().data()), AZ::UserSettings::CT_GLOBAL);

            m_ui->m_assetBrowserTableViewWidget->setVisible(false);
            if (ed_useNewAssetPickerView)
            {
                m_ui->m_assetBrowserTreeViewWidget->setVisible(false);
                m_ui->m_assetBrowserTableViewWidget->setVisible(true);
                m_tableModel->setSourceModel(m_filterModel.get());
                m_ui->m_assetBrowserTableViewWidget->setModel(m_tableModel.get());

                m_ui->m_assetBrowserTableViewWidget->SetName("AssetBrowserTableView_" + name);
                m_ui->m_assetBrowserTableViewWidget->setDragEnabled(false);
                m_ui->m_assetBrowserTableViewWidget->setSelectionMode(
                    selection.GetMultiselect() ? QAbstractItemView::SelectionMode::ExtendedSelection
                                               : QAbstractItemView::SelectionMode::SingleSelection);

                if (ed_hideAssetPickerPathColumn)
                {
                    m_ui->m_assetBrowserTableViewWidget->hideColumn(1);
                }

                // if the current selection is invalid, disable the Ok button
                m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(EvaluateSelection());

                connect(
                    m_filterModel.data(), &AssetBrowserFilterModel::filterChanged, this,
                    [this]()
                    {
                        m_tableModel->UpdateTableModelMaps();
                    });

                connect(
                    m_ui->m_assetBrowserTableViewWidget, &AssetBrowserTableView::selectionChangedSignal, this,
                    [this](const QItemSelection&, const QItemSelection&)
                    {
                        AssetPickerDialog::SelectionChangedSlot();
                    });

                connect(m_ui->m_assetBrowserTableViewWidget, &QAbstractItemView::doubleClicked, this, &AssetPickerDialog::DoubleClickedSlot);

                connect(
                    m_ui->m_assetBrowserTableViewWidget, &AssetBrowserTableView::ClearStringFilter, m_ui->m_searchWidget,
                    &SearchWidget::ClearStringFilter);

                connect(
                    m_ui->m_assetBrowserTableViewWidget, &AssetBrowserTableView::ClearTypeFilter, m_ui->m_searchWidget,
                    &SearchWidget::ClearTypeFilter);

                 connect(
                    this, &AssetPickerDialog::SizeChangedSignal, m_ui->m_assetBrowserTableViewWidget,
                    &AssetBrowserTableView::UpdateSizeSlot);

                m_ui->m_assetBrowserTableViewWidget->SetName("AssetBrowserTableView_main");
                m_tableModel->UpdateTableModelMaps();
            }

            QTimer::singleShot(0, this, &AssetPickerDialog::RestoreState);
            SelectionChangedSlot();
        }

        AssetPickerDialog::~AssetPickerDialog() = default;

        void AssetPickerDialog::accept()
        {
            SaveState();
            QDialog::accept();
        }

        void AssetPickerDialog::reject()
        {
            m_selection.GetResults().clear();
            SaveState();
            QDialog::reject();
        }

        void AssetPickerDialog::OnFilterUpdated()
        {
            if (!m_hasFilter)
            {
                m_filterStateSaver->CaptureSnapshot(m_ui->m_assetBrowserTreeViewWidget);
            }

            m_filterModel->filterUpdatedSlot();

            const bool hasFilter = m_ui->m_searchWidget->hasStringFilter();

            if (hasFilter)
            {
                // The update slot queues the update, so we need to react after that update.
                QTimer::singleShot(0, this, [this]()
                {
                    m_ui->m_assetBrowserTreeViewWidget->expandAll();
                });
                m_tableModel->UpdateTableModelMaps();
            }

            if (m_hasFilter && !hasFilter)
            {
                m_filterStateSaver->ApplySnapshot(m_ui->m_assetBrowserTreeViewWidget);
                m_hasFilter = false;
            }
            else if (!m_hasFilter && hasFilter)
            {
                m_hasFilter = true;
            }
        }

        void AssetPickerDialog::resizeEvent(QResizeEvent* resizeEvent)
        {
            emit SizeChangedSignal(m_ui->verticalLayout_4->geometry().width());
            QDialog::resizeEvent(resizeEvent);
        }

        void AssetPickerDialog::keyPressEvent(QKeyEvent* e)
        {
            // Until search widget is revised, Return key should not close the dialog,
            // it is used in search widget interaction
            if (e->key() == Qt::Key_Return)
            {
                if (EvaluateSelection())
                {
                    QDialog::accept();
                }
            }
            else
            {
                QDialog::keyPressEvent(e);
            }
        }

        bool AssetPickerDialog::EvaluateSelection() const
        {
            auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->isVisible() ? m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets()
                                                                                : m_ui->m_assetBrowserTableViewWidget->GetSelectedAssets();
            // exactly one item must be selected, even if multi-select option is disabled, still good practice to check
            if (selectedAssets.empty())
            {
                return false;
            }

            m_selection.GetResults().clear();

            for (auto entry : selectedAssets)
            {
                m_selection.GetSelectionFilter()->Filter(m_selection.GetResults(), entry);
                if (m_selection.IsValid() && !m_selection.GetMultiselect())
                {
                    break;
                }
            }
            return m_selection.IsValid();
        }

        void AssetPickerDialog::DoubleClickedSlot(const QModelIndex& index)
        {
            AZ_UNUSED(index);
            if (EvaluateSelection())
            {
                QDialog::accept();
            }
        }

        void AssetPickerDialog::UpdatePreview() const
        {
            auto selectedAssets = m_ui->m_assetBrowserTreeViewWidget->isVisible()
                ? m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets()
                : m_ui->m_assetBrowserTableViewWidget->GetSelectedAssets();
            ;
            if (selectedAssets.size() != 1)
            {
                m_ui->m_previewerFrame->Clear();
                return;
            }

            m_ui->m_previewerFrame->Display(selectedAssets.front());
        }

        void AssetPickerDialog::SaveState()
        {
            m_ui->m_assetBrowserTreeViewWidget->SaveState();
            if (m_persistentState)
            {
                m_persistentState->CaptureGeometry(parentWidget() ? parentWidget() : this);
            }
        }

        void AssetPickerDialog::RestoreState()
        {
            if (m_persistentState)
            {
                const auto widget = parentWidget() ? parentWidget() : this;
                m_persistentState->RestoreGeometry(widget);
            }
        }

        void AssetPickerDialog::SelectionChangedSlot()
        {
            m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(EvaluateSelection());
            UpdatePreview();
        }
    } // AssetBrowser
} // AzToolsFramework

#include <AssetBrowser/AssetPicker/moc_AssetPickerDialog.cpp>
