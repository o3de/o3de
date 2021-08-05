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
#include <AzCore/std/functional.h>

#include <QTreeView>
#include <QStyledItemDelegate>
#include <QColor>
#include <QPointer>
#endif

class QSettings;

namespace AzQtComponents
{
    class Style;
    class TableViewModel;

    /*!
     * Specialized TableView that allows the selected row to accordion out, so that all
     * of the text in the row is visible, with wrapping.
     * TableView inherits from QTreeView instead of QTableView as it provided more
     * customization in the subclass over the size of rows.
     *
     * Note that in order for the (accordion/)expandOnSelection functionality to work,
     * you must have a model set that eventually unproxies to a TableViewModel and a
     * delegate that is a TableViewItemDelegate.
     *
     */
    class AZ_QT_COMPONENTS_API TableView
        : public QTreeView
    {
        Q_OBJECT

        /*!
         * Set this property to true if you'd like the TableView to expand the selected
         * row to ensure that all of the text within is visible, with wrapping.
         * 
         * Note that in order for this functionality to work, you must have a model set
         * that eventually unproxies to a TableViewModel and a delegate that is a
         * TableViewItemDelegate.
         */
        Q_PROPERTY(bool expandOnSelection READ expandOnSelection WRITE setExpandOnSelection)

        friend class Style;

    public:
        struct Config
        {
            int borderWidth;
            QColor borderColor;
            qreal focusBorderWidth;
            QColor focusBorderColor;
            QColor focusFillColor;
            QColor headerFillColor;
        };

        /*!
         * Loads the TablveView config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default TableView config data.
         */
        static Config defaultConfig();

        explicit TableView(QWidget* parent = nullptr);

        // Enables the expansion of rows on selection.
        void setExpandOnSelection(bool expand = true);
        bool expandOnSelection() const;

        // Returns true if the data model (unproxied) inherits or is a TableViewModel
        bool dataModelSupportsIsSelectedRole() const;

    protected:
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

        void updateGeometries() override;

        void resizeEvent(QResizeEvent* e) override;

    private:
        static bool drawHeader(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawHeaderSection(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawFrameFocusRect(const Style* style, const QStyleOption* option, QPainter* painter, const Config& config);
        static QRect itemViewItemRect(const Style* style, QStyle::SubElement element, const QStyleOptionViewItem* option, const QWidget* widget, const Config& config);
        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget, const Config& config);

        static bool polish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const ScrollBar::Config& scrollBarConfig, const Config& config);

        TableViewModel* getTableViewModel() const;

        QModelIndex mapToTableViewModel(QModelIndex unmappedIndex) const;

        void selectRow(const QModelIndex& unmappedIndex);

        void handleResize();

        bool m_expandOnSelection = false;
        SelectionMode m_previousSelectionMode;
    };

    // A normal QAbstractTableModel with an extra method for notifying the model of changes on the selected row height
    // so that information can be provided back to the view through Qt::SizeHintRole
    class AZ_QT_COMPONENTS_API TableViewModel
        : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum TableViewModelRole
        {
            IsSelectedRole = Qt::UserRole + 1,
        };

        using QAbstractTableModel::QAbstractTableModel;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        int selectedRowHeight() const;

    private slots:
        void resetInternalData();

    private:
        friend class TableView;

        void setSelectedRowHeight(int selectedRow, int height);

        int m_selectedRow = -1;
        int m_selectedRowHeight = -1;
    };

    // A QStyledItemDelegate with an enriched interface to allow for single row resizing
    class AZ_QT_COMPONENTS_API TableViewItemDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT
        friend class TableView;

    public:
        TableViewItemDelegate(TableView* parent);

        virtual QRect itemViewItemRect(const AzQtComponents::Style* style, QStyle::SubElement element, const QStyleOptionViewItem* option, const QWidget* widget, const AzQtComponents::TableView::Config& config);

        virtual void geometriesUpdating(TableView* tableView);

    protected:

        virtual void clearCachedValues();

        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;

        int unexpandedRowHeight(const QModelIndex& index, const QWidget* view) const;
        virtual int sizeHintForSelectedIndex(const QModelIndex& mappedIndex, const TableView* view) const;

        // Use this function to check for is selected instead of going direct
        // because a custom data model that does not inherit from TableViewModel
        // will not necessarily support TableViewModel::IsSelectedRole.
        bool isSelected(const QModelIndex& unmappedIndex) const;

    private:
        int sizeHintForSelectedRow(const QModelIndex& mappedIndex, const TableView* view) const;

        mutable int m_unexpandedRowHeight = -1;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'AzQtComponents::TableViewItemDelegate::m_parentView': class 'QPointer<AzQtComponents::TableView>' needs to have dll-interface to be used by clients of class 'AzQtComponents::TableViewItemDelegate'
        QPointer<TableView> m_parentView;
        AZ_POP_DISABLE_WARNING
    };

} // namespace AzQtComponents
