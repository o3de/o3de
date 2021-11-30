/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <GemCatalog/GemItemDelegate.h>
#endif


namespace O3DE::ProjectManager
{
    class GemRequirementDelegate
        : public GemItemDelegate
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit GemRequirementDelegate(QAbstractItemModel* model, QObject* parent = nullptr);
        ~GemRequirementDelegate() = default;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) override;

        const QColor m_backgroundColor = QColor("#444444"); // Outside of the actual gem item
        const QColor m_itemBackgroundColor = QColor("#393939"); // Background color of the gem item

    private:
        QRect CalcRequirementRect(const QRect& contentRect) const;
    };
} // namespace O3DE::ProjectManager
