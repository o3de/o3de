/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QAbstractItemView>
#include <QColor>
#include <QRect>
#include <QSize>
#endif

class QPainter;
class QSettings;
class QUndoStack;

namespace AZ
{
    class Color;
}

namespace AzQtComponents
{
    namespace Internal
    {
        class ColorController;

        class AddSwatch : public QFrame
        {
            Q_OBJECT

        public:
            explicit AddSwatch(QWidget* parent = nullptr);

        signals:
            void clicked();

        protected:
            void paintEvent(QPaintEvent* event) override;
            void mouseReleaseEvent(QMouseEvent* event) override;
            void keyReleaseEvent(QKeyEvent* event) override;

        private:
            QSize m_iconSize = { 16, 16 };
            QIcon m_icon;
        };
    }

    class Style;
    class Palette;

    class PaletteModel;
    class PaletteItemDelegate;

    class AZ_QT_COMPONENTS_API PaletteView
        : public QAbstractItemView
    {
        Q_OBJECT
        Q_PROPERTY(bool gammaEnabled READ isGammaEnabled WRITE setGammaEnabled)
        Q_PROPERTY(qreal gamma READ gamma WRITE setGamma)

    public:
        struct DropIndicator
        {
            int width;
            QColor color;
        };

        struct Config
        {
            DropIndicator dropIndicator;
            QSize marginSize;
            int horizontalSpacing;
            int verticalSpacing;
        };

        /*!
         * Loads the palette view config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default palette view config data.
         */
        static Config defaultConfig();

        static bool polish(Style* style, QWidget* widget, const Config& config);

        static bool drawDropIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);


        explicit PaletteView(Palette* palette, Internal::ColorController* controller, QUndoStack* undoStack, QWidget* parent = nullptr);

        ~PaletteView() override;

        bool isGammaEnabled() const;
        qreal gamma() const;

        void addDefaultRGBColors();

        void setModel(QAbstractItemModel* model) override;
        void setSelectionModel(QItemSelectionModel* selectionModel) override;

        QModelIndex indexAt(const QPoint& point) const override;
        void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
        QRect visualRect(const QModelIndex& index) const override;

        void setMarginSize(const QSize& size);
        void setItemSize(const QSize& size);
        void setItemHorizontalSpacing(int spacing);
        void setItemVerticalSpacing(int spacing);

        bool contains(const AZ::Color& color) const;
        
        QMenu* contextMenu() const { return m_context; }

    public Q_SLOTS:
        void setGammaEnabled(bool enabled);
        void setGamma(qreal gamma);
        void tryAddColor(const AZ::Color& color);

    protected Q_SLOTS:
        void handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    Q_SIGNALS:
        void selectedColorsChanged(const QVector<AZ::Color>& selectedColors);
        void unselectedContextMenuRequested(const QPoint& position);

    protected:
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        int horizontalOffset() const override;
        int verticalOffset() const override;
        QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
        void updateGeometries() override;
        bool isIndexHidden(const QModelIndex& index) const override;
        void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags) override;
        QRegion visualRegionForSelection(const QItemSelection& selection) const override;

        void paintEvent(QPaintEvent* event) override;
        void rowsInserted(const QModelIndex& parent, int start, int end) override;
        void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
        void resizeEvent(QResizeEvent *event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

        void startDrag(Qt::DropActions supportedActions) override;

        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dropEvent(QDropEvent *event) override;

    private:
        QPoint itemPosition(int index) const;
        QRect itemGeometry(int index) const;
        void updateItemLayout();

        void removeSelection();
        void cut();
        void copy() const;
        void paste();

        bool canDrop(QDropEvent* event);
        bool canDrop(QDropEvent* event, QModelIndex& index, int& row);
        QRect getDropIndicatorRect(QModelIndex index, int row) const;
        int getRootRow(const QPoint point, const QModelIndex index) const;

        void onColorChanged(const AZ::Color& c);

        PaletteModel* m_paletteModel;
        PaletteItemDelegate* m_delegate;
        Internal::ColorController* m_controller;
        QUndoStack* m_undoStack;

        bool m_gammaEnabled = false;
        qreal m_gamma = 1.0;

        int m_itemRows;
        int m_itemColumns;
        QSize m_preferredSize;
        QSize m_itemSize;
        int m_itemHSpacing;
        int m_itemVSpacing;

        bool m_droppingOn;
        QRect m_dropIndicatorRect;

        Internal::AddSwatch* m_addColorButton;

        QMenu* m_context;
        QAction* m_cut;
        QAction* m_copy;
        QAction* m_paste;
        QAction* m_remove;
        QAction* m_update;

        bool m_aboutToShowContextMenu = false;
    };

} // namespace AzQtComponents
