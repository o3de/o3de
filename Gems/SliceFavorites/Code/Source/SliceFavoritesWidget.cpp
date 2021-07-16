/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliceFavoritesWidget.h"
#include "FavoriteDataModel.h"

#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QTimer>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>

#include <Source/ui_SliceFavoritesWidget.h>

Q_DECLARE_METATYPE(AZ::Uuid);
namespace SliceFavorites
{
    SliceFavoritesWidget::SliceFavoritesWidget(FavoriteDataModel* dataModel, QWidget* pParent, Qt::WindowFlags flags)
        : QWidget(pParent, flags)
        , m_gui(new Ui::SliceFavoritesWidgetUI())
        , m_dataModel(dataModel)
    {
        m_gui->setupUi(this);

        m_gui->emptyLabel->setContextMenuPolicy(Qt::CustomContextMenu);

        m_gui->treeView->setModel(dataModel);
        m_gui->treeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_gui->treeView->setDragEnabled(true);
        m_gui->treeView->setDropIndicatorShown(true);
        m_gui->treeView->setDragDropMode(QAbstractItemView::DragDrop);
        m_gui->treeView->setAcceptDrops(true);
        m_gui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_gui->treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_gui->treeView->setHeaderHidden(true);

        connect(m_gui->treeView, &QTreeView::customContextMenuRequested, this, &SliceFavoritesWidget::OnOpenTreeContextMenu);
        
        m_removeAction = new QAction(QObject::tr("Remove"), this);
        m_removeAction->setShortcut(QKeySequence(Qt::Key_Delete));
        m_gui->treeView->addAction(m_removeAction);
        connect(m_removeAction, &QAction::triggered, this, &SliceFavoritesWidget::RemoveSelection);

        connect(m_dataModel, &FavoriteDataModel::DataModelChanged, this, &SliceFavoritesWidget::UpdateWidget);
        connect(m_dataModel, &FavoriteDataModel::ExpandIndex, m_gui->treeView, &QTreeView::setExpanded);
        connect(m_dataModel, &FavoriteDataModel::DisplayWarning, this, [this](const QString& title, const QString& message)
        {
            QMessageBox::warning(this, title, message);
        });

        connect(m_gui->emptyLabel, &QLabel::customContextMenuRequested, this, &SliceFavoritesWidget::OnOpenTreeContextMenu);

        UpdateWidget();
    }

    SliceFavoritesWidget::~SliceFavoritesWidget()
    {
    }

    void SliceFavoritesWidget::UpdateWidget()
    {
        bool modelHasFavoritesOrFolders = m_dataModel->HasFavoritesOrFolders();

       m_gui->emptyLabel->setText(modelHasFavoritesOrFolders ? "" : "Right click to add folders or use the Asset Browser to add slices as favorites");
       m_gui->emptyLabel->setVisible(!modelHasFavoritesOrFolders);
    }

    void SliceFavoritesWidget::RemoveSelection()
    {
        const QItemSelection& currentSelection = m_gui->treeView->selectionModel()->selection();

        if (currentSelection.indexes().size() > 0)
        {
            int numFoldersToRemove = 0;
            int numFavoritesToRemove = 0;

            m_dataModel->CountFoldersAndFavoritesFromIndices(currentSelection.indexes(), numFoldersToRemove, numFavoritesToRemove);

            QString outputString = QStringLiteral("Are you sure you want to remove the following?\n%1 folders\n%2 favorites").arg(numFoldersToRemove).arg(numFavoritesToRemove);

            QMessageBox msgBox(this);
            msgBox.setText(tr("Confirm removal"));
            msgBox.setInformativeText(outputString);
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            msgBox.setIcon(QMessageBox::NoIcon);
            int result = msgBox.exec();

            if (result == QMessageBox::Ok)
            {
                // Only delete the highest-level items from the current selection
                // Their children will be automatically removed
                AZStd::vector<QModelIndex> toDelete;
                for (const auto& selectedIndex : currentSelection.indexes())
                {
                    bool child = false;
                    for (const auto& potentialAncestor : currentSelection.indexes())
                    {
                        if (m_dataModel->IsDescendentOf(selectedIndex, potentialAncestor))
                        {
                            child = true;
                            break;
                        }
                    }   

                    if (!child)
                    {
                        toDelete.push_back(selectedIndex);
                    }
                }

                for (const auto& selectedIndex : toDelete)
                {
                    if (selectedIndex.isValid())
                    {
                        m_dataModel->RemoveFavorite(selectedIndex);
                    }
                }
            }
        }
    }

    void SliceFavoritesWidget::AddNewFolder(const QModelIndex& currentIndex)
    {
        QModelIndex newFolderIndex = m_dataModel->AddNewFolder(currentIndex);

        // Select the new folder and start editing it by default
        m_gui->treeView->selectionModel()->select(newFolderIndex, QItemSelectionModel::ClearAndSelect);
        m_gui->treeView->selectionModel()->setCurrentIndex(newFolderIndex, QItemSelectionModel::ClearAndSelect);

        m_gui->treeView->edit(newFolderIndex);
    }

    void SliceFavoritesWidget::OnOpenTreeContextMenu(const QPoint& pos)
    {
        const QItemSelection& currentSelection = m_gui->treeView->selectionModel()->selection();

        QMenu* contextMenu = new QMenu(m_gui->treeView);
        contextMenu->setToolTipsVisible(true);

        QModelIndex firstSelection = currentSelection.indexes().size() > 0 ? currentSelection.indexes().first() : QModelIndex();

        QAction* renameAction = contextMenu->addAction("Rename", contextMenu, [this, &firstSelection]()
        {
            m_gui->treeView->edit(firstSelection);
        });
        renameAction->setEnabled(currentSelection.indexes().size() == 1);
        renameAction->setToolTip(tr("Rename the favorite or folder, not the slice itself"));

        contextMenu->addSeparator();

        QAction* folderAction = contextMenu->addAction("Add folder", contextMenu, [this, &firstSelection]()
        {
            AddNewFolder(firstSelection);
        });
        folderAction->setEnabled(CanAddNewFolder(currentSelection));

        contextMenu->addSeparator();

        QAction* removeAction = contextMenu->addAction("Remove selected", contextMenu, [this]()
        {
            RemoveSelection();
        });
        removeAction->setEnabled(currentSelection.indexes().size() > 0);

        int numFavoritesAndFolders = m_dataModel->GetNumFavoritesAndFolders();

        QAction* clearAction = contextMenu->addAction("Remove all", contextMenu, [this]()
        {
            QMessageBox msgBox(this);
            msgBox.setText(tr("Confirm removal"));
            msgBox.setInformativeText(tr("Remove all favorites?"));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            msgBox.setIcon(QMessageBox::NoIcon);
            int result = msgBox.exec();

            if (result == QMessageBox::Ok)
            {
                m_dataModel->ClearAll();
            }

            
        });
        clearAction->setEnabled(numFavoritesAndFolders > 0);

        contextMenu->addSeparator();

        contextMenu->addAction("Import slice...", contextMenu, [this]()
        {
            AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Slice");
            AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
            if (selection.IsValid())
            {
                auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
                if (product)
                {
                    m_dataModel->AddFavorite(product);
                }
            }
        });

        contextMenu->addSeparator();

        contextMenu->addAction("Import slice favorites...", contextMenu, [this]()
        {
            QString fileName = QFileDialog::getOpenFileName(this, "Import Favorites From...", QString(), tr("XML (*.xml)"), nullptr, QFileDialog::DontUseNativeDialog);
            if (fileName.length() > 0)
            {
                int numImported = m_dataModel->ImportFavorites(fileName);
                QString outputString = QStringLiteral("%1 favorites and folders successfully imported.").arg(numImported);
                QMessageBox::information(this, "Import successful!", outputString);
            }
        });

        QAction* exportAction = contextMenu->addAction("Export slice favorites...", contextMenu, [this]()
        {
            QString fileName = QFileDialog::getSaveFileName(nullptr, QString("Export Favorites To..."), "SliceFavorites.xml", QString("XML (*.xml)"));
            if (fileName.length() > 0)
            {
                int numExported = m_dataModel->ExportFavorites(fileName);
                QString outputString = QStringLiteral("%1 favorites and folders successfully exported.").arg(numExported);
                QMessageBox::information(this, "Export successful!", outputString);
            }
        });
        exportAction->setEnabled(numFavoritesAndFolders > 0);

        contextMenu->exec(m_gui->treeView->mapToGlobal(pos));
        delete contextMenu;
    }

    bool SliceFavoritesWidget::CanAddNewFolder(const QItemSelection& selected)
    {
        // We can add a folder if we have exactly one or fewer selected items
        // we check item type later for appropriate types
        bool allowAddFolder = (selected.size() <= 1);

        for (const auto& selectedIndex : selected.indexes())
        {
            FavoriteData* favoriteData = m_dataModel->GetFavoriteDataFromModelIndex(selectedIndex);
            if (favoriteData)
            {
                // We can only add folders to other folders, not to favorites themselves
                allowAddFolder &= (favoriteData->m_type == FavoriteData::DataType_Folder);
            }
        }

        return allowAddFolder;
    }
}
#include <Source/moc_SliceFavoritesWidget.cpp>
