/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#if !defined(Q_MOC_RUN)
#include <QStyledItemDelegate>
#include <QMetaType>
#include <QSvgRenderer>
#include <AzCore/std/string/string.h>
#endif

namespace AssetProcessor
{
    struct GoToButtonData
    {
        GoToButtonData() = default;

        GoToButtonData(AZStd::string destination)
            : m_destination(AZStd::move(destination))
        {
        }

        AZStd::string m_destination;
    };

    class GoToButtonDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT
    public:

        GoToButtonDelegate(QObject* parent = nullptr);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    Q_SIGNALS:
        void Clicked(const GoToButtonData& data);

    protected:

        QIcon m_icon;
        QIcon m_hoverIcon;
    };
} // namespace AssetProcessor

Q_DECLARE_METATYPE(AssetProcessor::GoToButtonData);
