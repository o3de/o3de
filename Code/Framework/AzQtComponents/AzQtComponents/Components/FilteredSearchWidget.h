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

#include <QScopedPointer>
#include <QFrame>
#include <QStyledItemDelegate>
#include <QMap>
#include <QVariant>
#include <QMenu>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QStandardItem>

#include <AzCore/std/chrono/chrono.h>
#endif

namespace Ui
{
    class FilteredSearchWidget;
}

class FlowLayout;
class QTreeView;
class QStandardItemModel;
class QSettings;
class QLineEdit;
class QToolButton;
class QLabel;
class QHBoxLayout;
class QIcon;
class QBoxLayout;

namespace AzQtComponents
{
    class Style;
    class FilteredSearchItemDelegate;
    class SearchTypeSelectorFilterModel;

    class AZ_QT_COMPONENTS_API FilterCriteriaButton
        : public QFrame
    {
        Q_OBJECT

    public:
        enum class ExtraButtonType
        {
            None,
            Locked,
            Unlocked,
            Visible,
        };

        explicit FilterCriteriaButton(const QString& labelText, QWidget* parent = nullptr, FilterCriteriaButton::ExtraButtonType type = FilterCriteriaButton::ExtraButtonType::None, const QString& extraIconFileName = QString());

    protected:
        QHBoxLayout* m_frameLayout;
        QLabel* m_tagLabel;

    signals:
        void RequestClose();
        void ExtraButtonClicked(FilterCriteriaButton::ExtraButtonType type);
    };

    struct AZ_QT_COMPONENTS_API SearchTypeFilter
    {
        QString category;
        QString displayName;
        QString extraIconFilename;
        QVariant metadata;
        int globalFilterValue;
        bool enabled = false;
        FilterCriteriaButton::ExtraButtonType typeExtraButton = FilterCriteriaButton::ExtraButtonType::None;

        SearchTypeFilter() {}
        SearchTypeFilter(const QString& category, const QString& displayName, FilterCriteriaButton::ExtraButtonType type = FilterCriteriaButton::ExtraButtonType::None, const QString& extraIconFilename = QString(), const QVariant& metadata = {}, int globalFilterValue = -1)
            : category(category)
            , displayName(displayName)
            , extraIconFilename(extraIconFilename)
            , metadata(metadata)
            , globalFilterValue(globalFilterValue)
            , typeExtraButton(type)
        {
        }
    };

    using SearchTypeFilterList = QVector<SearchTypeFilter>;

    class SearchTypeSelectorTreeView;

    class AZ_QT_COMPONENTS_API SearchTypeSelector : public QMenu
    {
        Q_OBJECT

        // SearchTypeSelector popup menus are fixed width. Set the fixed width via this if you want something non-default.
        // In a stylesheet, set it this way:
        //
        // qproperty-fixedWidth: yourIntegerVirtualPixelValueHere;
        //
        Q_PROPERTY(int fixedWidth READ fixedWidth WRITE setFixedWidth)

        // SearchTypeSelector popup menus make a decent guess of how big the contents are in order to properly
        // position the menu above or below the parent button. But even with the guess, there is configurable
        // padding not taken into account. That extra padding along the bottom can be tweaked by setting
        // the heightEstimatePadding value.
        //
        // In a stylesheet, set it this way:
        //
        // qproperty-heightEstimatePadding: yourIntegerVirtualPixelValueHere;
        //
        Q_PROPERTY(int heightEstimatePadding READ heightEstimatePadding WRITE setHeightEstimatePadding)

        // Set this to false in order to hide the text filter, usually on small numbers of items.
        // Defaults to true.
        //
        // In a stylesheet, set it this way:
        //
        // qproperty-lineEditSearchVisible: 0;
        //
        Q_PROPERTY(bool lineEditSearchVisible READ lineEditSearchVisible WRITE setLineEditSearchVisible)

        // The margin to apply around the line edit search field's layout.
        // Defaults to 4.
        //
        // In a stylesheet, set it this way:
        //
        // qproperty-searchLayoutMargin: 0;
        //
        Q_PROPERTY(int searchLayoutMargin READ searchLayoutMargin WRITE setSearchLayoutMargin)

    public:
        SearchTypeSelector(QWidget* parent = nullptr);
        QTreeView* GetTree();
        void Setup(const SearchTypeFilterList& searchTypes);

        int fixedWidth() const { return m_fixedWidth; }
        void setFixedWidth(int newFixedWidth);

        int heightEstimatePadding() const { return m_heightEstimatePadding; }
        void setHeightEstimatePadding(int newHeightEstimatePadding);

        bool lineEditSearchVisible() const;
        void setLineEditSearchVisible(bool visible);

        int searchLayoutMargin() const;
        void setSearchLayoutMargin(int newMargin);

        const QString& GetFilterString() const { return m_filterString; }
    signals:
        void TypeToggled(int id, bool enabled);

    private slots:
        void FilterTextChanged(const QString& newFilter);

    protected:
        void estimateTableHeight(int numCategories, int numItems);

        // allows child classes to override the logic of accepting filter categories
        virtual bool filterItemOut(const QModelIndex& sourceIndex, bool filteredByBase) { Q_UNUSED(sourceIndex); return filteredByBase; }
        virtual void initItem(QStandardItem* item, const SearchTypeFilter& filter, int unfilteredDataIndex);
        int getUnfilteredDataIndex(QStandardItem* item); // get the original filter index from the item itself

        // Returns the number of items that always appear in the list, regardless of the filtering.
        virtual int GetNumFixedItems() { return 0; }

        void showEvent(QShowEvent* e) override;

        bool eventFilter(QObject* obj, QEvent* event) override;

        void RepopulateDataModel(const SearchTypeFilterList& unfilteredData);
        void maximizeGeometryToFitScreen();

        SearchTypeSelectorTreeView* m_tree;
        QStandardItemModel* m_model;

        friend class SearchTypeSelectorFilterModel;
        SearchTypeSelectorFilterModel* m_filterModel;
        QString m_filterString;
        bool m_settingUp = false;
        int m_fixedWidth = 256;
        QLineEdit* m_searchField = nullptr;
        QBoxLayout* m_searchLayout = nullptr;
        int m_estimatedTableHeight = 0;
        int m_estimatedTableWidth = 256;
        int m_heightEstimatePadding = 10;
        int m_searchLayoutMargin = 4;
        bool m_lineEditSearchVisible = true;
    };

    class SearchTypeSelectorFilterModel : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        SearchTypeSelectorFilterModel(SearchTypeSelector* searchTypeSelector);
        void setNoResultsMessageRow(int row); // row of specialized "no results" message in the source model

    protected slots:
        void onRowCountChanged();

    protected:
        bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
        int getNumLeafNodes(const QModelIndex& theIndex = QModelIndex()); // gets the number of leaf node descendants of current index. Current index is *not* considered

        SearchTypeSelector* m_searchTypeSelector = nullptr;
        int m_noResultsRow = -1; // row of specialized "no results" message in the source model
        bool m_showingNoResultsMessage = false;
    };

    class AZ_QT_COMPONENTS_API FilteredSearchWidget
        : public QFrame
    {
        Q_OBJECT
            Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY placeholderTextChanged)
            Q_PROPERTY(QString textFilter READ textFilter WRITE SetTextFilter NOTIFY TextFilterChanged)
            Q_PROPERTY(bool textFilterFillsWidth READ textFilterFillsWidth WRITE setTextFilterFillsWidth NOTIFY textFilterFillsWidthChanged)

    public:
        struct Config
        {
        };

        /*!
         * Loads the button config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default button config data.
         */
        static Config defaultConfig();

        explicit FilteredSearchWidget(QWidget* parent = nullptr, bool willUseOwnSelector = false);
        ~FilteredSearchWidget() override;

        void SetTypeFilterVisible(bool visible);
        void SetTypeFilters(const SearchTypeFilterList& typeFilters);
        void AddTypeFilter(const SearchTypeFilter& typeFilter);
        void SetupOwnSelector(SearchTypeSelector* selector);

        inline void AddTypeFilter(const QString& category, const QString& displayName, const QVariant& metadata = {}, int globalFilterValue = -1, FilterCriteriaButton::ExtraButtonType type = FilterCriteriaButton::ExtraButtonType::None, const QString& extraIconFileName = {})
        {
            AddTypeFilter(SearchTypeFilter(category, displayName, type, extraIconFileName, metadata, globalFilterValue));
        }

        void SetTextFilterVisible(bool visible);
        void SetTextFilter(const QString& textFilter);
        void ClearTextFilter();

        void AddWidgetToSearchWidget(QWidget* w);
        void SetFilteredParentVisible(bool visible);
        void setEnabledFiltersVisible(bool visible);

        void SetFilterState(const QString& category, const QString& displayName, bool enabled);
        void SetFilterInputInterval(AZStd::chrono::milliseconds milliseconds);

        QString placeholderText() const;
        void setPlaceholderText(const QString& placeholderText);

        QString textFilter() const;
        bool hasStringFilter() const;

        bool textFilterFillsWidth() const;
        void setTextFilterFillsWidth(bool fillsWidth);

        void clearLabelText();
        void setLabelText(const QString& newLabelText);
        QString labelText() const;

        static QString GetBackgroundColor();
        static QString GetSeparatorColor();

        QToolButton* assetTypeSelectorButton() const;
    signals:
        void TextFilterChanged(const QString& activeTextFilter);
        void TypeFilterChanged(const SearchTypeFilterList& activeTypeFilters);

        void placeholderTextChanged(const QString& placeholderText);
        void textFilterFillsWidthChanged(bool fillsWidth);

    public slots:
        virtual void ClearTypeFilter();

        virtual void SetFilterStateByIndex(int index, bool enabled);

        void SetFilterState(int index, bool enabled) { SetFilterStateByIndex(index, enabled); }

        void readSettings(QSettings& settings, const QString& widgetName);
        void writeSettings(QSettings& settings, const QString& widgetName);

    protected:
        void emitTypeFilterChanged();
        QLineEdit* filterLineEdit() const;
        QToolButton* filterTypePushButton() const;
        SearchTypeSelector* filterTypeSelector() const;
        const SearchTypeFilterList& typeFilters() const;

        virtual FilterCriteriaButton* createCriteriaButton(const SearchTypeFilter& filter, int filterIndex);

        virtual void SetupPaintDelegates();
    private slots:
        void UpdateTextFilterWidth();
        void OnClearFilterContextMenu(const QPoint& pos);
        void OnSearchContextMenu(const QPoint& pos);

        void OnTextChanged(const QString& activeTextFilter);
        void UpdateTextFilter();

    protected:
        AZ_PUSH_DISABLE_WARNING(4127 4251, "-Wunknown-warning-option") // conditional expression is constant, needs to have dll-interface to be used by clients of class 'AzQtComponents::FilteredSearchWidget'
            SearchTypeFilterList m_typeFilters;
        AZ_POP_DISABLE_WARNING
            FlowLayout* m_flowLayout;
        Ui::FilteredSearchWidget* m_ui;
        SearchTypeSelector* m_selector;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // needs to have dll-interface to be used by clients of class 'AzQtComponents::FilteredSearchWidget'
            QMap<int, FilterCriteriaButton*> m_typeButtons;
        AZ_POP_DISABLE_WARNING
            bool m_textFilterFillsWidth;
        bool m_displayEnabledFilters;

    private:
        int FindFilterIndex(const QString& category, const QString& displayName) const;

        QTimer m_inputTimer;
        QMenu* m_filterMenu;

        static const char* s_filterDataProperty;

        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);

        FilteredSearchItemDelegate* m_delegate = nullptr;
    };

    class FilteredSearchItemDelegate : public QStyledItemDelegate
    {
    public:
        explicit FilteredSearchItemDelegate(QWidget* parent = nullptr);

        void PaintRichText(QPainter* painter, QStyleOptionViewItem& opt, QString& text) const;
        void SetSelector(SearchTypeSelector* selector) { m_selector = selector; }

        // QStyledItemDelegate overrides.
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        SearchTypeSelector* m_selector = nullptr;
    };
}
