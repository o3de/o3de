/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <UI/PropertyEditor/View/AssetCompleterListView.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QPainter>
#include <QTextDocument>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    AssetCompleterDelegate::AssetCompleterDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
    }

    void AssetCompleterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItem styledOption = option;
        initStyleOption(&styledOption, index);

        QStyle* style = option.widget ? option.widget->style() : QApplication::style();

        QTextDocument doc;
        doc.setHtml(styledOption.text);

        styledOption.text = QString();

        style->drawControl(QStyle::CE_ItemViewItem, &styledOption, painter);

        QAbstractTextDocumentLayout::PaintContext ctx;

        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &styledOption);
        painter->save();
        painter->translate(textRect.topLeft());
        painter->setClipRect(textRect.translated(-textRect.topLeft()));

        doc.documentLayout()->draw(painter, ctx);
        painter->restore();
    }
    
    QSize AssetCompleterDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem o = option;
        initStyleOption(&o, index);

        QTextDocument doc;
        doc.setHtml(o.text);
        doc.setTextWidth(o.rect.width());
        return QSize((int)doc.idealWidth(), s_rowHeight);
    }

    AssetCompleterListView::AssetCompleterListView(QWidget* parent)
        : QListView(parent)
        , m_delegate(nullptr)
    {
        m_delegate = new AssetCompleterDelegate(this);
        setItemDelegateForColumn(1, m_delegate);
    }

    AssetCompleterListView::~AssetCompleterListView()
    {
        delete m_delegate;
    }

    void AssetCompleterListView::SelectFirstItem() 
    {
        setCurrentIndex(model()->index(0, 0));
    }
}

#include <UI/PropertyEditor/View/moc_AssetCompleterListView.cpp>
