/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GemCatalog/GemInfo.h>
#include <GemCatalog/GemModel.h>
#include <GemsSubWidget.h>
#include <LinkWidget.h>

#include <QItemSelection>
#include <QScrollArea>
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace O3DE::ProjectManager
{
    class GemInspector
        : public QScrollArea
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemInspector(GemModel* model, QWidget* parent = nullptr);
        ~GemInspector() = default;

        void Update(const QModelIndex& modelIndex);
        static QLabel* CreateStyledLabel(QLayout* layout, int fontSize, const QString& colorCodeString);

        // Colors
        inline constexpr static const char* s_headerColor = "#FFFFFF";
        inline constexpr static const char* s_textColor = "#DDDDDD";

    private slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        void InitMainWidget();

        GemModel* m_model = nullptr;
        QWidget* m_mainWidget = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;

        // General info (top) section
        QLabel* m_nameLabel = nullptr;
        QLabel* m_creatorLabel = nullptr;
        QLabel* m_summaryLabel = nullptr;
        LinkLabel* m_directoryLinkLabel = nullptr;
        LinkLabel* m_documentationLinkLabel = nullptr;

        // Requirements
        QLabel* m_reqirementsTitleLabel = nullptr;
        QLabel* m_reqirementsIconLabel = nullptr;
        QLabel* m_reqirementsTextLabel = nullptr;

        // Depending and conflicting gems
        GemsSubWidget* m_dependingGems = nullptr;

        // Additional information
        QLabel* m_versionLabel = nullptr;
        QLabel* m_lastUpdatedLabel = nullptr;
        QLabel* m_binarySizeLabel = nullptr;
    };
} // namespace O3DE::ProjectManager
