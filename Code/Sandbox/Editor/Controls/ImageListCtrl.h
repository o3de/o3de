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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_CONTROLS_IMAGELISTCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_IMAGELISTCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractItemView>

#include <QHash>
#endif

//////////////////////////////////////////////////////////////////////////
// Custom control to display list of images.
//////////////////////////////////////////////////////////////////////////
class CImageListCtrl
    : public QAbstractItemView
{
    Q_OBJECT
public:
    enum ListStyle
    {
        DefaultStyle,
        HorizontalStyle
    };

public:
    CImageListCtrl(QWidget* parent = nullptr);
    ~CImageListCtrl();

    ListStyle Style() const;
    void SetStyle(ListStyle style);

    const QSize& ItemSize() const;
    void SetItemSize(QSize size);

    const QSize& BorderSize() const;
    void SetBorderSize(QSize size);

    // Get all items inside specified rectangle.
    QModelIndexList ItemsInRect(const QRect& rect) const;

    QModelIndex indexAt(const QPoint& point) const override;
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
    QRect visualRect(const QModelIndex& index) const override;

protected:
    QRect ItemGeometry(const QModelIndex& index) const;
    void SetItemGeometry(const QModelIndex& index, const QRect& rect);
    void ClearItemGeometries();

    int horizontalOffset() const override;
    int verticalOffset() const override;
    bool isIndexHidden(const QModelIndex& index) const override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags) override;
    QRegion visualRegionForSelection(const QItemSelection& selection) const override;

    void paintEvent(QPaintEvent* event) override;
    void rowsInserted(const QModelIndex& parent, int start, int end) override;

    void updateGeometries() override;

private:
    QHash<int, QRect> m_geometry;
    QSize m_itemSize;
    QSize m_borderSize;
    ListStyle m_style;
};

class QImageListDelegate
    : public QAbstractItemDelegate
{
    Q_OBJECT
signals:
    void InvalidPixmapGenerated(const QModelIndex& index) const;
public:
    QImageListDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;

    QVector<int> paintingRoles() const override;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_IMAGELISTCTRL_H
