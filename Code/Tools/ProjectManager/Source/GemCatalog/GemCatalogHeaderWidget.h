/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/function/function_fwd.h>

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/SearchLineEdit.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <TagWidget.h>
#include <QFrame>
#include <QLabel>
#include <QDialog>
#include <QMoveEvent>
#include <QHideEvent>
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

    private:
        QStringList ConvertFromModelIndices(const QVector<QModelIndex>& gems) const;

        using GetTagIndicesCallback = AZStd::function<QVector<QModelIndex>()>;
        void CreateGemSection(const QString& singularTitle, const QString& pluralTitle, GetTagIndicesCallback getTagIndices);

        QVBoxLayout* m_layout = nullptr;
        GemModel* m_gemModel = nullptr;

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
        void hideEvent(QHideEvent*) override;

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
