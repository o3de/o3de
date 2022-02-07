/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "OutlinerSearchWidget.h"

#include <AzCore/Casting/numeric_cast.h>
#include <AzQtComponents/Components/FlowLayout.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <QLabel>
#include <QTreeView>
#include <QTextDocument>
#include <QPainter>
#include <QToolButton>
#include <QApplication>

namespace AzQtComponents
{
    OutlinerIcons::OutlinerIcons()
    {
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Unlocked)] = QIcon(QString(":Icons/unlocked.svg"));
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Locked)] = QIcon(QString(":Icons/locked.svg"));
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Visible)] = QIcon(QString(":Icons/visb.svg"));
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Hidden)] = QIcon(QString(":Icons/visb_hidden.svg"));
        m_globalIcons[static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::Separator)] = QIcon();
    }

    OutlinerSearchTypeSelector::OutlinerSearchTypeSelector(QWidget* parent)
        : SearchTypeSelector(parent)
    {
    }

    bool OutlinerSearchTypeSelector::filterItemOut(const QModelIndex& sourceIndex, bool filteredByBase)
    {
        auto* currItem = m_model->itemFromIndex(sourceIndex);
        if (currItem != nullptr)
        {
            int unfilteredIndex = getUnfilteredDataIndex(currItem);
            if (unfilteredIndex >= 0 && unfilteredIndex < aznumeric_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
            {
                // never filter out the categories before FirstRealFilter (unlocked/locked, visible/hidden, etc.)
                return false;
            }
        }
        // no special case, return the result of the base filter
        return filteredByBase;
    }

    void OutlinerSearchTypeSelector::initItem(QStandardItem* item, const SearchTypeFilter& filter, int unfilteredDataIndex)
    {
        SearchTypeSelector::initItem(item, filter, unfilteredDataIndex);
        if (filter.displayName == "--------")
        {
            item->setCheckable(false);
        }

        if (unfilteredDataIndex >= 0 && unfilteredDataIndex < static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
        {
            item->setIcon(OutlinerIcons::GetInstance().GetIcon(unfilteredDataIndex));
        }
    }

    int OutlinerSearchTypeSelector::GetNumFixedItems()
    {
        return static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter);
    }

    OutlinerCriteriaButton::OutlinerCriteriaButton(QString labelText, QWidget* parent, int index)
        : FilterCriteriaButton(labelText, parent)
    {
        if (index >= 0 && index < static_cast<int>(OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
        {
            QLabel* icon = new QLabel(this);
            icon->setStyleSheet(m_tagLabel->styleSheet() + "border: 0px; background-color: transparent;");
            icon->setPixmap(OutlinerIcons::GetInstance().GetIcon(index).pixmap(10, 10));
            m_frameLayout->insertWidget(0, icon);
        }
    }

    OutlinerSearchWidget::OutlinerSearchWidget(QWidget* parent)
        : FilteredSearchWidget(parent, true)
    {
        SetupOwnSelector(new OutlinerSearchTypeSelector(assetTypeSelectorButton()));

        const SearchTypeFilterList globalMenu{
            {"Global Settings", "Unlocked"},
            {"Global Settings", "Locked"},
            {"Global Settings", "Visible"},
            {"Global Settings", "Hidden"},
            {"Global Settings", "--------"}
        };
        int value = 0;
        for (const SearchTypeFilter& filter : globalMenu)
        {
            AddTypeFilter(filter.category, filter.displayName, QVariant::fromValue<AZ::Uuid>(AZ::Uuid::Create()), value);
            ++value;
        }
    }

    OutlinerSearchWidget::~OutlinerSearchWidget()
    {
        delete m_delegate;
        m_delegate = nullptr;

        delete m_selector;
        m_selector = nullptr;
    }

    void OutlinerSearchWidget::SetupPaintDelegates()
    {
        m_delegate = new OutlinerSearchItemDelegate(m_selector->GetTree());
        m_selector->GetTree()->setItemDelegate(m_delegate);
        m_delegate->SetSelector(m_selector);
    }

    FilterCriteriaButton* OutlinerSearchWidget::createCriteriaButton(const SearchTypeFilter& filter, int filterIndex)
    {
        return new OutlinerCriteriaButton(filter.displayName, this, filterIndex);
    }

    OutlinerSearchItemDelegate::OutlinerSearchItemDelegate(QWidget* parent) : QStyledItemDelegate(parent)
    {
    }

    void OutlinerSearchItemDelegate::PaintRichText(QPainter* painter, QStyleOptionViewItem& opt, QString& text) const
    {
        int textDocDrawYOffset = 3;
        QPoint paintertextDocRenderOffset = QPoint(-2, -1);

        QTextDocument textDoc;
        textDoc.setDefaultFont(opt.font);
        opt.palette.color(QPalette::Text);
        textDoc.setDefaultStyleSheet("body {color: " + opt.palette.color(QPalette::Text).name() + "}");
        textDoc.setHtml("<body>" + text + "</body>");
        QRect textRect = opt.widget->style()->proxy()->subElementRect(QStyle::SE_ItemViewItemText, &opt);
        painter->translate(textRect.topLeft() - paintertextDocRenderOffset);
        textDoc.setTextWidth(textRect.width());
        textDoc.drawContents(painter, QRectF(0, textDocDrawYOffset, textRect.width(), textRect.height() + textDocDrawYOffset));
    }

    void OutlinerSearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const
    {
        bool isGlobalOption = false;
        painter->save();

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const QWidget* widget = option.widget;
        QStyle* style = widget ? widget->style() : QApplication::style();
        if (!opt.icon.isNull())
        {
            // Draw the icon if there is one.
            QRect r = style->subElementRect(QStyle::SubElement::SE_ItemViewItemDecoration, &opt, widget);
            r.setX(-r.width());

            QIcon::Mode mode = QIcon::Normal;
            QIcon::State state = QIcon::On;
            opt.icon.paint(painter, r, opt.decorationAlignment, mode, state);

            opt.icon = QIcon();
            opt.decorationSize = QSize(0, 0);
            isGlobalOption = true;
        }

        // Handle the separator
        if (opt.text.contains("-------"))
        {
            // Draw this item as a solid line.
            painter->setPen(QColor(FilteredSearchWidget::GetSeparatorColor()));
            painter->drawLine(0, opt.rect.center().y() + 3, opt.rect.right(), opt.rect.center().y() + 4);
        }
        else
        {
            if (m_selector->GetFilterString().length() > 0 && !isGlobalOption && opt.features & QStyleOptionViewItem::ViewItemFeature::HasCheckIndicator)
            {
                // Create rich text menu text to show filterstring
                QString label{ opt.text };
                opt.text = "";

                style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

                int highlightTextIndex = 0;
                do
                {
                    // Find filter term within the text.
                    highlightTextIndex = label.lastIndexOf(m_selector->GetFilterString(), highlightTextIndex - 1, Qt::CaseInsensitive);
                    if (highlightTextIndex >= 0)
                    {
                        // Insert background-color terminator at appropriate place to return to normal text.
                        label.insert(highlightTextIndex + m_selector->GetFilterString().length(), "</span>");
                        // Insert background-color command at appropriate place to highlight filter term.
                        label.insert(highlightTextIndex, "<span style=\"background-color: " + FilteredSearchWidget::GetBackgroundColor() + "\">");
                    }
                } while (highlightTextIndex > 0);// Repeat in case there are multiple occurrences.
                PaintRichText(painter, opt, label);  
            }
            else
            {          
                // There's no filter to apply, just draw it.
                QString label = opt.text;
                opt.text = "";
                style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
                PaintRichText(painter, opt, label);
            }
        }

        painter->restore();
    }

    QSize OutlinerSearchItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        if (opt.text.contains("-------"))
        {
            return QSize(0, 6);
        }
        return QStyledItemDelegate::sizeHint(option, index);
    }

}

#include <UI/Outliner/moc_OutlinerSearchWidget.cpp>
