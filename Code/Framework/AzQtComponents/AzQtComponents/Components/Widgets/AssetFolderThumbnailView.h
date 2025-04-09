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
#include <AzQtComponents/Components/Widgets/ScrollBar.h>

AZ_PUSH_DISABLE_WARNING(4244, "-Wunknown-warning-option")
#include <QAbstractItemView>
#include <QStyledItemDelegate>
AZ_POP_DISABLE_WARNING
#endif

class QSettings;
class QTimer;

namespace AzQtComponents
{
    class Style;
    class AssetFolderThumbnailViewDelegate;

    class AZ_QT_COMPONENTS_API AssetFolderThumbnailView
        : public QAbstractItemView
    {
        Q_OBJECT
        Q_PROPERTY(ThumbnailSize thumbnailSize READ thumbnailSize WRITE setThumbnailSize)

    public:
        struct Config
        {
            struct Thumbnail
            {
                int width;
                int height;
                qreal borderRadius;
                int padding;
                QColor backgroundColor;
                qreal borderThickness;
                qreal selectedBorderThickness;
                QColor borderColor;
                QColor selectedBorderColor;
            };

            struct ExpandButton
            {
                int width;
                qreal borderRadius;
                qreal caretWidth;
                qreal caretHeight;
                QColor backgroundColor;
                QColor caretColor;
            };

            struct ChildFrame
            {
                int padding;
                qreal borderRadius;
                QColor borderColor;
                QColor backgroundColor;
                int closeButtonWidth;
            };

            int viewportPadding;
            int topItemsHorizontalSpacing;
            int topItemsVerticalSpacing;
            int childrenItemsHorizontalSpacing;
            int scrollSpeed;
            Thumbnail rootThumbnail;
            Thumbnail childThumbnail;
            ExpandButton expandButton;
            ChildFrame childFrame;
        };

        static Config loadConfig(QSettings& settings);

        static Config defaultConfig();

        explicit AssetFolderThumbnailView(QWidget* parent = nullptr);
        ~AssetFolderThumbnailView() override;

        enum class ThumbnailSize
        {
            Small,
            Medium,
            Large
        };
        Q_ENUM(ThumbnailSize)

        enum class Role
        {
            IsExpandable = Qt::UserRole + 1000,
            IsTopLevel,
            IsExactMatch,
            IsVisible,
        };

        void setThumbnailSize(ThumbnailSize size);
        ThumbnailSize thumbnailSize() const;

        void rowsInserted(const QModelIndex& parent, int start, int end) override;
        void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
        void reset() override;
        void updateGeometries() override;
        QModelIndex indexAt(const QPoint& point) const override;
        void scrollTo(const QModelIndex& index, QAbstractItemView::ScrollHint hint) override;
        QRect visualRect(const QModelIndex& index) const override;

        void setRootIndex(const QModelIndex &index) override;

        void SetShowSearchResultsMode(bool searchMode);
        bool InSearchResultsMode() const;

    protected Q_SLOTS:
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

    signals:
        void rootIndexChanged(const QModelIndex& idx);
        void contextMenu(const QModelIndex& idx);
        void afterRename(const QString& value) const;
        void deselected();
        void selectionChangedSignal(const QItemSelection& selected, const QItemSelection& deselected);

    protected:
        friend class Style;

        static bool polish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config);
        void polish(const Config& config);

        QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
        int horizontalOffset() const override;
        int verticalOffset() const override;
        bool isIndexHidden(const QModelIndex& index) const override;
        void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags) override;
        QRegion visualRegionForSelection(const QItemSelection& selection) const override;

        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        void paintChildFrames(QPainter* painter) const;
        void paintItems(QPainter* painter) const;

        bool isExpandable(const QModelIndex& index) const;

        int rootThumbnailSizeInPixels() const;
        int childThumbnailSizeInPixels() const;

        void updateGeometriesInternal(const QModelIndex& index, int& x, int& y);

        QModelIndex indexAtPos(const QPoint& pos) const;

        AssetFolderThumbnailViewDelegate* m_delegate;   
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'AzQtComponents::AssetFolderThumbnailView::m_itemGeometry': class 'QHash<QPersistentModelIndex,QRect>' needs to have dll-interface to be used by clients of class 'AzQtComponents::AssetFolderThumbnailView'
        QHash<QPersistentModelIndex, QRect> m_itemGeometry;
        struct ChildFrame
        {
            QPersistentModelIndex index;
            QVector<QRect> rects;
        };
        QVector<ChildFrame> m_childFrames;
        QSet<QPersistentModelIndex> m_expandedIndexes;
        AZ_POP_DISABLE_WARNING
        ThumbnailSize m_thumbnailSize;
        Config m_config;
        bool m_showSearchResultsMode = false;
        bool m_hideProductAssets = true;

        // Selection Handling
        void SelectAllEntitiesInSelectionRect();
        QItemSelection m_previousSelection;

        void ClearQueuedMouseEvent();
        void ProcessQueuedMousePressedEvent(QMouseEvent* event);
        void HandleDrag();
        void StartCustomDrag(const QModelIndexList& indexList, Qt::DropActions supportedActions);
        QImage CreateDragImage(const QModelIndexList& indexList);

        QMouseEvent* m_queuedMouseEvent = nullptr;
        QPoint m_mousePosition;
        bool m_isDragSelectActive = false;
        QTimer* m_selectionUpdater;

        const int m_selectionUpdateInterval = 30; //msec
        const QColor m_dragSelectRectColor = QColor(255, 255, 255, 20);
        const QColor m_dragSelectBorderColor = QColor(255, 255, 255);
    };

    class AssetFolderThumbnailViewDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit AssetFolderThumbnailViewDelegate(QObject* parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        void polish(const AssetFolderThumbnailView::Config& config);

    signals:
        void RenameThumbnail(const QString& value) const;
    
    protected Q_SLOTS:
        void editingFinished();

    private:
        AssetFolderThumbnailView::Config m_config;
    };
}
