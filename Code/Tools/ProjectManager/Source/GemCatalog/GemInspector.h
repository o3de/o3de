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
#endif

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSpacerItem)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace O3DE::ProjectManager
{
    class GemInspector
        : public QScrollArea
    {
        Q_OBJECT

    public:
        explicit GemInspector(GemModel* model, QWidget* parent = nullptr);
        ~GemInspector() = default;

        void Update(const QModelIndex& modelIndex);
        static QLabel* CreateStyledLabel(QLayout* layout, int fontSize, const QString& colorCodeString);

        // Fonts
        inline constexpr static int s_baseFontSize = 12;

        // Colors
        inline constexpr static const char* s_headerColor = "#FFFFFF";
        inline constexpr static const char* s_textColor = "#DDDDDD";

    signals:
        void TagClicked(const Tag& tag);
        void UpdateGem(const QModelIndex& modelIndex);
        void UninstallGem(const QModelIndex& modelIndex);
        void EditGem(const QModelIndex& modelIndex);

    private slots:
        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        void InitMainWidget();

        GemModel* m_model = nullptr;
        QWidget* m_mainWidget = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;
        QModelIndex m_curModelIndex;

        // General info (top) section
        QLabel* m_nameLabel = nullptr;
        QLabel* m_creatorLabel = nullptr;
        QLabel* m_summaryLabel = nullptr;
        QLabel* m_licenseLabel = nullptr;
        LinkLabel* m_licenseLinkLabel = nullptr;
        LinkLabel* m_directoryLinkLabel = nullptr;
        LinkLabel* m_documentationLinkLabel = nullptr;

        // Requirements
        QLabel* m_requirementsTitleLabel = nullptr;
        QLabel* m_requirementsIconLabel = nullptr;
        QLabel* m_requirementsTextLabel = nullptr;
        QSpacerItem* m_requirementsMainSpacer = nullptr;

        // Depending gems
        GemsSubWidget* m_dependingGems = nullptr;
        QSpacerItem* m_dependingGemsSpacer = nullptr;

        // Additional information
        QLabel* m_versionLabel = nullptr;
        QLabel* m_lastUpdatedLabel = nullptr;
        QLabel* m_binarySizeLabel = nullptr;

        QPushButton* m_updateGemButton = nullptr;
        QPushButton* m_editGemButton = nullptr;
        QPushButton* m_uninstallGemButton = nullptr;
    };
} // namespace O3DE::ProjectManager
