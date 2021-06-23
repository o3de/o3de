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
    };
} // namespace O3DE::ProjectManager
