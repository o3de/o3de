/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GemRepo/GemRepoModel.h>
#include <LinkWidget.h>
#include <GemsSubWidget.h>

#include <QItemSelection>
#include <QScrollArea>
#include <QSpacerItem>
#include <QWidget>
#include <QPersistentModelIndex>
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QItemSelectionModel)

namespace AzQtComponents
{
    class ElidingLabel;
}

namespace O3DE::ProjectManager
{
    class GemRepoInspector : public QScrollArea
    {
        Q_OBJECT

    public:

        explicit GemRepoInspector(GemRepoModel* model, QItemSelectionModel* selectionModel, QWidget* parent = nullptr);
        ~GemRepoInspector() = default;

        void Update(const QModelIndex& modelIndex);

    signals:
        void RemoveRepo(const QModelIndex& modelIndex);
        void ShowToastNotification(const QString& notification);

    private slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnCopyDownloadLinkClicked();

    private:
        void InitMainWidget();


        GemRepoModel* m_model = nullptr;
        QItemSelectionModel* m_selectionModel = nullptr;
        QWidget* m_mainWidget = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;

        // General info section
        AzQtComponents::ElidingLabel* m_nameLabel = nullptr;
        LinkLabel* m_repoLinkLabel = nullptr;
        LinkLabel* m_copyDownloadLinkLabel = nullptr;
        QLabel* m_summaryLabel = nullptr;

        // Additional information
        QLabel* m_addInfoTitleLabel = nullptr;
        QLabel* m_addInfoTextLabel = nullptr;
        QSpacerItem* m_addInfoSpacer = nullptr;

        // Buttons
        QPushButton* m_removeRepoButton = nullptr;

        // Included objects 
        GemsSubWidget* m_includedGems = nullptr;
        GemsSubWidget* m_includedProjects = nullptr;
        GemsSubWidget* m_includedTemplates = nullptr;

        QModelIndex m_curModelIndex; 
    };
} // namespace O3DE::ProjectManager
