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
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace O3DE::ProjectManager
{
    class GemRepoInspector : public QScrollArea
    {
        Q_OBJECT // AUTOMOC

            public : explicit GemRepoInspector(GemRepoModel* model, QWidget* parent = nullptr);
        ~GemRepoInspector() = default;

        void Update(const QModelIndex& modelIndex);

    private slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        void InitMainWidget();

        GemRepoModel* m_model = nullptr;
        QWidget* m_mainWidget = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;

        // General info section
        QLabel* m_nameLabel = nullptr;
        LinkLabel* m_repoLinkLabel = nullptr;
        QLabel* m_summaryLabel = nullptr;

        // Additional information
        QLabel* m_addInfoTitleLabel = nullptr;
        QLabel* m_addInfoTextLabel = nullptr;
        QSpacerItem* m_addInfoSpacer = nullptr;

        // Included Gems
        GemsSubWidget* m_includedGems = nullptr;
    };
} // namespace O3DE::ProjectManager
