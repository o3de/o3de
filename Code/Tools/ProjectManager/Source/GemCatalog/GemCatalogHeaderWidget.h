/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/function/function_fwd.h>
#include <AzQtComponents/Components/SearchLineEdit.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <TagWidget.h>
#include <DownloadController.h>

#include <QFrame>
#include <QScrollArea>
#endif

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QHideEvent)
QT_FORWARD_DECLARE_CLASS(QMoveEvent)
QT_FORWARD_DECLARE_CLASS(QMovie)

namespace O3DE::ProjectManager
{
    class GemCartWidget
        : public QScrollArea
    {
        Q_OBJECT // AUTOMOC

    public:
        GemCartWidget(GemModel* gemModel, DownloadController* downloadController, QWidget* parent = nullptr);
        ~GemCartWidget();

    public slots:
        void GemDownloadAdded(const QString& gemName);
        void GemDownloadRemoved(const QString& gemName);
        void GemDownloadProgress(const QString& gemName, int bytesDownloaded, int totalBytes);

    private:
        QVector<Tag> GetTagsFromModelIndices(const QVector<QModelIndex>& gems) const;

        using GetTagIndicesCallback = AZStd::function<QVector<QModelIndex>()>;
        void CreateGemSection(const QString& singularTitle, const QString& pluralTitle, GetTagIndicesCallback getTagIndices);
        void CreateDownloadSection();
        void OnCancelDownloadActivated(const QString& link);

        QVBoxLayout* m_layout = nullptr;
        GemModel* m_gemModel = nullptr;
        DownloadController* m_downloadController = nullptr;

        QWidget* m_downloadSectionWidget = nullptr;
        QWidget* m_downloadingListWidget = nullptr;

        inline constexpr static int s_width = 240;
    };

    class CartButton
        : public QWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        CartButton(GemModel* gemModel, DownloadController* downloadController, QWidget* parent = nullptr);
        ~CartButton();
        void ShowGemCart();

    signals:
        void UpdateGemCart(QWidget* gemCart);

    private:
        void mousePressEvent(QMouseEvent* event) override;
        void hideEvent(QHideEvent*) override;

        GemModel* m_gemModel = nullptr;
        QHBoxLayout* m_layout = nullptr;
        QLabel* m_countLabel = nullptr;
        QPushButton* m_dropDownButton = nullptr;
        GemCartWidget* m_gemCart = nullptr;
        DownloadController* m_downloadController = nullptr;

        inline constexpr static int s_iconSize = 24;
        inline constexpr static int s_arrowDownIconSize = 8;
    };

    class GemCatalogHeaderWidget
        : public QFrame
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemCatalogHeaderWidget(GemModel* gemModel, GemSortFilterProxyModel* filterProxyModel, DownloadController* downloadController, QWidget* parent = nullptr);
        ~GemCatalogHeaderWidget() = default;

        void ReinitForProject();

    public slots:
        void GemDownloadAdded(const QString& gemName);
        void GemDownloadRemoved(const QString& gemName);
        void GemCartShown(bool state = false);

    signals:
        void AddGem();
        void OpenGemsRepo();
        void RefreshGems();
        void UpdateGemCart(QWidget* gemCart);

    protected slots:
        void paintEvent(QPaintEvent* event) override;
        
    private:
        AzQtComponents::SearchLineEdit* m_filterLineEdit = nullptr;
        inline constexpr static int s_height = 60;
        DownloadController* m_downloadController = nullptr;
        QLabel* m_downloadSpinner = nullptr;
        QLabel* m_downloadLabel = nullptr;
        QMovie* m_downloadSpinnerMovie = nullptr;
        CartButton* m_cartButton = nullptr;
        bool m_showGemCart = false;
    };
} // namespace O3DE::ProjectManager
