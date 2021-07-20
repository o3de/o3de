/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Casting/numeric_cast.h>

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <QStyledItemDelegate>
#include <QStandardItem>
#endif

#if !defined(DEFINED_QMETATYPE_UUID)
#define DEFINED_QMETATYPE_UUID
#include <AzCore/Math/Uuid.h>

Q_DECLARE_METATYPE(AZ::Uuid);
#endif

namespace Ui
{
    class EntityOutlinerSearchWidget;
}

namespace AzToolsFramework
{
    class EntityOutlinerSearchItemDelegate;

    class EntityOutlinerSearchTypeSelector
        : public AzQtComponents::SearchTypeSelector
    {
    public:
        EntityOutlinerSearchTypeSelector(QWidget* parent = nullptr);

    protected:
        // override the logic of accepting filter categories
        bool filterItemOut(const QModelIndex& sourceIndex, bool filteredByBase) override;
        void initItem(QStandardItem* item, const AzQtComponents::SearchTypeFilter& filter, int unfilteredDataIndex) override;
        int GetNumFixedItems() override;
    };

    class OutlinerCriteriaButton
        : public AzQtComponents::FilterCriteriaButton
    {
        Q_OBJECT

    public:
        explicit OutlinerCriteriaButton(QString labelText, QWidget* parent = nullptr, int index = -1);
    };

    class EntityOutlinerSearchWidget
        : public AzQtComponents::FilteredSearchWidget
    {
        Q_OBJECT
    public:
        explicit EntityOutlinerSearchWidget(QWidget* parent = nullptr);
        ~EntityOutlinerSearchWidget() override;

        AzQtComponents::FilterCriteriaButton* createCriteriaButton(const AzQtComponents::SearchTypeFilter& filter, int filterIndex) override;

        enum class GlobalSearchCriteria : int
        {
            Unlocked,
            Locked,
            Visible,
            Hidden,
            Separator,
            FirstRealFilter
        };
    protected:
        void SetupPaintDelegates() override;
    private:
        EntityOutlinerSearchItemDelegate* m_delegate = nullptr;
    };

    class EntityOutlinerIcons
    {
    public:
        static EntityOutlinerIcons& GetInstance()
        {
            static EntityOutlinerIcons instance;
            return instance;
        }
        EntityOutlinerIcons(EntityOutlinerIcons const&) = delete;
        void operator=(EntityOutlinerIcons const&) = delete;

        QIcon& GetIcon(int iconWanted) { return m_globalIcons[iconWanted]; }
    private:
        EntityOutlinerIcons();

        QIcon m_globalIcons[aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter)];
    };

    class EntityOutlinerSearchItemDelegate : public QStyledItemDelegate
    {
    public:
        explicit EntityOutlinerSearchItemDelegate(QWidget* parent = nullptr);

        void PaintRichText(QPainter* painter, QStyleOptionViewItem& opt, QString& text) const;
        void SetSelector(AzQtComponents::SearchTypeSelector* selector) { m_selector = selector; }

        // QStyleItemDelegate overrides.
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        AzQtComponents::SearchTypeSelector* m_selector = nullptr;
    };
}
