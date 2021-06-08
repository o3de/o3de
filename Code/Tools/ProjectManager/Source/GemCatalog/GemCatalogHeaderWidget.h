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

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/SearchLineEdit.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <TagWidget.h>
#include <QFrame>
#include <QLabel>
#include <QDialog>
#include <QMoveEvent>
#include <QVBoxLayout>
#endif

namespace O3DE::ProjectManager
{
    class CartOverlayWidget
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        CartOverlayWidget(GemModel* gemModel, QWidget* parent = nullptr);
        void Update();

    private:
        QStringList ConvertFromModelIndices(const QVector<QModelIndex>& gems) const;

        QVBoxLayout* m_layout = nullptr;
        GemModel* m_gemModel = nullptr;

        QWidget* m_enabledWidget = nullptr;
        QLabel* m_enabledLabel = nullptr;
        TagContainerWidget* m_enabledTagContainer = nullptr;

        QWidget* m_disabledWidget = nullptr;
        QLabel* m_disabledLabel = nullptr;
        TagContainerWidget* m_disabledTagContainer = nullptr;

        inline constexpr static int s_width = 240;
    };

    class CartButton
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        CartButton(GemModel* gemModel, QWidget* parent = nullptr);
        ~CartButton();
        void ShowOverlay();

    private:
        void mousePressEvent(QMouseEvent* event) override;

        GemModel* m_gemModel = nullptr;
        QHBoxLayout* m_layout = nullptr;
        QLabel* m_countLabel = nullptr;
        QPushButton* m_dropDownButton = nullptr;
        CartOverlayWidget* m_cartOverlay = nullptr;

        inline constexpr static int s_iconSize = 24;
        inline constexpr static int s_arrowDownIconSize = 8;
    };

    class GemCatalogHeaderWidget
        : public QFrame
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemCatalogHeaderWidget(GemModel* gemModel, GemSortFilterProxyModel* filterProxyModel, QWidget* parent = nullptr);
        ~GemCatalogHeaderWidget() = default;

        void ReinitForProject();

    private:
        AzQtComponents::SearchLineEdit* m_filterLineEdit = nullptr;
        inline constexpr static int s_height = 60;
    };
} // namespace O3DE::ProjectManager
