/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityOutlinerSearchWidget.h"

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Uuid.h>

#include <AzQtComponents/Components/FlowLayout.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QApplication>
#include <QLabel>
#include <QPainter>
#include <QTextDocument>
#include <QToolButton>
#include <QTreeView>

namespace AzToolsFramework
{
    EntityOutlinerIcons::EntityOutlinerIcons()
    {
        m_globalIcons[aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Unlocked)] = QIcon(QString(":Icons/unlocked.svg"));
        m_globalIcons[aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Locked)] = QIcon(QString(":Icons/locked.svg"));
        m_globalIcons[aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Visible)] = QIcon(QString(":Icons/visb.svg"));
        m_globalIcons[aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Hidden)] = QIcon(QString(":Icons/visb_hidden.svg"));
        m_globalIcons[aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Separator)] = QIcon();
    }

    EntityOutlinerSearchTypeSelector::EntityOutlinerSearchTypeSelector(QWidget* parent)
        : SearchTypeSelector(parent)
    {
    }

    bool EntityOutlinerSearchTypeSelector::filterItemOut(const QModelIndex& sourceIndex, bool filteredByBase)
    {
        auto* currItem = m_model->itemFromIndex(sourceIndex);
        if (currItem != nullptr)
        {
            int unfilteredIndex = getUnfilteredDataIndex(currItem);
            if (unfilteredIndex >= 0 && unfilteredIndex < aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
            {
                // never filter out the categories before FirstRealFilter (unlocked/locked, visible/hidden, etc.)
                return false;
            }
        }
        // no special case, return the result of the base filter
        return filteredByBase;
    }

    void EntityOutlinerSearchTypeSelector::initItem(QStandardItem* item, const AzQtComponents::SearchTypeFilter& filter, int unfilteredDataIndex)
    {
        SearchTypeSelector::initItem(item, filter, unfilteredDataIndex);
        if (filter.displayName == "--------")
        {
            SearchTypeSelector::initItem(item, filter, unfilteredDataIndex);
            item->setCheckable(false);
        }

        if (unfilteredDataIndex < aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
        {
            item->setIcon(EntityOutlinerIcons::GetInstance().GetIcon(unfilteredDataIndex));
        }
    }

    int EntityOutlinerSearchTypeSelector::GetNumFixedItems()
    {
        return aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter);
    }

    OutlinerCriteriaButton::OutlinerCriteriaButton(QString labelText, QWidget* parent, int index)
        : FilterCriteriaButton(labelText, parent)
    {
        if (index >= 0 && index < aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter))
        {
            QLabel* icon = new QLabel(this);
            icon->setStyleSheet(m_tagLabel->styleSheet() + "border: 0px; background-color: transparent;");
            icon->setPixmap(EntityOutlinerIcons::GetInstance().GetIcon(index).pixmap(10, 10));
            m_frameLayout->insertWidget(0, icon);
        }
    }

    EntityOutlinerSearchWidget::EntityOutlinerSearchWidget(QWidget* parent)
        : FilteredSearchWidget(parent, true)
    {
        SetupOwnSelector(new EntityOutlinerSearchTypeSelector(assetTypeSelectorButton()));

        const AzQtComponents::SearchTypeFilterList globalMenu{
            {"Global Settings", "Unlocked"},
            {"Global Settings", "Locked"},
            {"Global Settings", "Visible"},
            {"Global Settings", "Hidden"},
            {"Global Settings", "--------"}
        };
        int value = 0;
        for (const AzQtComponents::SearchTypeFilter& filter : globalMenu)
        {
            AddTypeFilter(filter.category, filter.displayName, QVariant::fromValue<AZ::Uuid>(AZ::Uuid::Create()), value);
            ++value;
        }
    }

    EntityOutlinerSearchWidget::~EntityOutlinerSearchWidget()
    {
        delete m_delegate;
        m_delegate = nullptr;

        delete m_selector;
        m_selector = nullptr;
    }

    void EntityOutlinerSearchWidget::SetupPaintDelegates()
    {
        m_delegate = new EntityOutlinerSearchItemDelegate(m_selector->GetTree());
        m_selector->GetTree()->setItemDelegate(m_delegate);
        m_delegate->SetSelector(m_selector);
    }

    AzQtComponents::FilterCriteriaButton* EntityOutlinerSearchWidget::createCriteriaButton(const AzQtComponents::SearchTypeFilter& filter, int filterIndex)
    {
        return new OutlinerCriteriaButton(filter.displayName, this, filterIndex);
    }

    EntityOutlinerSearchItemDelegate::EntityOutlinerSearchItemDelegate(QWidget* parent) : QStyledItemDelegate(parent)
    {
    }

    void EntityOutlinerSearchItemDelegate::PaintRichText(QPainter* painter, QStyleOptionViewItem& opt, QString& text) const
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

    void EntityOutlinerSearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
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
            painter->setPen(QColor(AzQtComponents::FilteredSearchWidget::GetSeparatorColor()));
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
                        label.insert(highlightTextIndex, "<span style=\"background-color: " + AzQtComponents::FilteredSearchWidget::GetBackgroundColor() + "\">");
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

    QSize EntityOutlinerSearchItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
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

#include <UI/Outliner/moc_EntityOutlinerSearchWidget.cpp>
