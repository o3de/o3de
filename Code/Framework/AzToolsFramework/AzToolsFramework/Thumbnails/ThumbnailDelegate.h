/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QStyledItemDelegate>

#include <AzCore/std/string/string.h>

class QWidget;
class QPainter;
class QStyleOptionViewItem;
class QAbstractItemModel;
class QModelIndex;

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //! Thumbnail delegate can be used as within item views to draw thumbnails
        class ThumbnailDelegate
            : public QStyledItemDelegate
        {
            Q_OBJECT
        public:
            explicit ThumbnailDelegate(QWidget* parent = nullptr);
            ~ThumbnailDelegate() override;

            //////////////////////////////////////////////////////////////////////////
            // QStyledItemDelegate
            //////////////////////////////////////////////////////////////////////////
            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            void setEditorData(QWidget* editor, const QModelIndex& index) const override;
            void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
            //! Set location where thumbnails are searched
            void SetThumbnailContext(const char* thumbnailContext);

        private:
            AZStd::string m_thumbnailContext;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
