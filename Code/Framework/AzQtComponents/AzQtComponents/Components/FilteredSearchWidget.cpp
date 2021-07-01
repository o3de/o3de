/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Utilities/ScreenUtilities.h>

#include <AzQtComponents/Components/ui_FilteredSearchWidget.h>

#include <AzQtComponents/Components/FlowLayout.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <QApplication>
#include <QMenu>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QScopedValueRollback>
#include <QScreen>
#include <QDesktopWidget>
#include <QTimer>
#include <QSettings>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyle>

#include <QTextDocument>

Q_DECLARE_METATYPE(AZ::Uuid);

namespace AzQtComponents
{
    static const QString g_textFilterKey = QStringLiteral("textFilter");
    static const QString g_typeFiltersKey = QStringLiteral("typeFilters");
    static const QString g_categoryKey = QStringLiteral("category");
    static const QString g_displayNameKey = QStringLiteral("displayName");
    static const QString g_enabledKey = QStringLiteral("enabled");

    static const int FilterWindowWidthPadding = 100;// Amount to add to filter window to account for initial spacing and scrollbar.

    const char* FilteredSearchWidget::s_filterDataProperty = "filterData";
    const char* const s_filterSearchWidgetName = "filteredSearchWidget";
    const char* const s_searchLayout = "searchLayout";
    const char* const s_tagText = "tagText";
    const char* const s_tagIcon = "tagIcon";

    const QString BackgroundColor{ "#565A5B" };
    const QString SeparatorColor{ "#606060" };

    FilterCriteriaButton::FilterCriteriaButton(const QString& labelText, QWidget* parent, FilterCriteriaButton::ExtraButtonType type, const QString& extraIconFileName)
        : QFrame(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_frameLayout = new QHBoxLayout(this);
        m_frameLayout->setMargin(0);
        m_frameLayout->setContentsMargins(4, 1, 0, 1);
        m_frameLayout->setSpacing(0);

        if (type != FilterCriteriaButton::ExtraButtonType::None)
        {
            QPushButton* extraButton = new QPushButton(this);
            switch (type)
            {
            case FilterCriteriaButton::ExtraButtonType::None:
                break;
            case FilterCriteriaButton::ExtraButtonType::Locked:
                extraButton->setObjectName("locked");
                break;
            case FilterCriteriaButton::ExtraButtonType::Unlocked:
                extraButton->setObjectName("unlocked");
                break;
            case FilterCriteriaButton::ExtraButtonType::Visible:
                extraButton->setObjectName("visible");
                break;
            }
            extraButton->setFlat(true);
            extraButton->setProperty("iconButton", "true");
            extraButton->setMouseTracking(true);
            connect(extraButton, &QPushButton::clicked, this, [this, type]() { emit ExtraButtonClicked(type); });
            m_frameLayout->addWidget(extraButton);
        }

        if (!extraIconFileName.isEmpty())
        {
            QLabel* extraIcon = new QLabel(this);
            extraIcon->setObjectName(s_tagIcon);
            extraIcon->setPixmap(QIcon(extraIconFileName).pixmap(16, 16));
            m_frameLayout->addWidget(extraIcon);
        }

        m_tagLabel = new QLabel(this);
        m_tagLabel->setObjectName(s_tagText);
        m_tagLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_tagLabel->setMinimumSize(24, 16);
        m_tagLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_tagLabel->setText(labelText);

        QPushButton* button = new QPushButton(this);
        button->setObjectName("closeTag");
        button->setFlat(true);
        button->setProperty("iconButton", "true");
        button->setMouseTracking(true);

        m_frameLayout->addWidget(m_tagLabel);
        m_frameLayout->addWidget(button);

        connect(button, &QPushButton::clicked, this, &FilterCriteriaButton::RequestClose);
    }

    // Custom class to get at the protected method rowHeight()
    class SearchTypeSelectorTreeView : public QTreeView
    {
    public:
        using QTreeView::QTreeView;

        int fetchRowHeight(const QModelIndex &index) const { return rowHeight(index); }
    };

    SearchTypeSelector::SearchTypeSelector(QWidget* parent /* = nullptr */)
        : QMenu(parent)
        , m_unfilteredData(nullptr)
    {
        Q_ASSERT(parent != nullptr);

        setObjectName("SearchTypeSelector");

        // Force an update if the stylesheet reloads; for some reason, this doesn't automatically happen without this
        auto style = qApp->style();
        if (auto ui20Stlye = qobject_cast<Style*>(style))
        {
            ui20Stlye->repolishOnSettingsChange(this);
        }

        QStyleOption options;
        options.init(this);
        const int hmargin = style->pixelMetric(QStyle::PM_MenuHMargin, &options, this);
        const int vmargin = style->pixelMetric(QStyle::PM_MenuVMargin, &options, this);

        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QStringLiteral("vertLayout"));
        verticalLayout->setContentsMargins(hmargin, vmargin, hmargin, vmargin);

        //make text search box
        m_searchLayout = new QVBoxLayout();
        m_searchLayout->setObjectName(s_searchLayout);
        m_searchLayout->setContentsMargins(m_searchLayoutMargin, m_searchLayoutMargin, m_searchLayoutMargin, m_searchLayoutMargin);
        
        QLineEdit* textSearch = new QLineEdit(this);
        m_searchField = textSearch;
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(textSearch->sizePolicy().hasHeightForWidth());
        textSearch->setSizePolicy(sizePolicy);
        textSearch->setMinimumSize(QSize(0, 25));
        textSearch->setObjectName(s_filterSearchWidgetName);
        textSearch->setFrame(false);
        textSearch->setText(QString());
        textSearch->setPlaceholderText(QObject::tr("Search..."));
        textSearch->setClearButtonEnabled(true);
        LineEdit::applySearchStyle(textSearch);
        connect(textSearch, &QLineEdit::textChanged, this, &SearchTypeSelector::FilterTextChanged);

        m_searchLayout->addWidget(textSearch);

        QVBoxLayout* itemLayout = new QVBoxLayout();
        itemLayout->setContentsMargins(0, 0, 0, 0);

        //add in item tree
        m_tree = new SearchTypeSelectorTreeView(this);
        m_tree->setAlternatingRowColors(false);
        m_tree->setRootIsDecorated(true);
        m_tree->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding));
        m_tree->setMinimumSize(QSize(0, 1));
        m_tree->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);

        itemLayout->addWidget(m_tree);

        m_model = new QStandardItemModel(this);
        m_tree->setModel(m_model);
        m_tree->setHeaderHidden(true);

        connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item)
        {
            // Don't emit itemChanged while Setup is running
            if (!m_settingUp)
            {
                int index = item->data().toInt();
                if (index < m_filteredItemIndices.size())
                {
                    index = m_filteredItemIndices[item->data().toInt()];
                }
                
                bool enabled = item->checkState() == Qt::Checked;
                emit TypeToggled(index, enabled);
            }
        });
        verticalLayout->addLayout(m_searchLayout);
        verticalLayout->addLayout(itemLayout);
    }

    void SearchTypeSelector::showEvent(QShowEvent* e)
    {
        // Have to do this here, because there's no other way to inject code between
        // QPushButton _q_popupPressed and QMenu::exec, and QMenu does all kinds
        // of strange stuff to resize and reposition the menu
        maximizeGeometryToFitScreen();

        QMenu::showEvent(e);
    }

    void SearchTypeSelector::resetData()
    {
        m_estimatedTableHeight = 0;
        m_estimatedTableWidth = 0;

        m_filteredItemIndices.clear();
        m_model->clear();
    }

    void SearchTypeSelector::initItem(QStandardItem* item, const SearchTypeFilter& filter, int unfilteredDataIndex)
    {
        Q_UNUSED(filter);
        Q_UNUSED(unfilteredDataIndex);

        item->setCheckable(true);
        item->setCheckState(filter.enabled ? Qt::Checked : Qt::Unchecked);
    }

    bool SearchTypeSelector::filterItemOut(int unfilteredDataIndex, bool itemMatchesFilter, bool categoryMatchesFilter)
    {
        Q_UNUSED(unfilteredDataIndex);

        return !itemMatchesFilter && !categoryMatchesFilter;
    }

    void SearchTypeSelector::RepopulateDataModel()
    {
        resetData();

        if (!m_unfilteredData)
        {
            return;
        }

        bool amFiltering = !m_filterString.isEmpty();

        QScopedValueRollback<bool> setupGuard(m_settingUp, true);
        QMap<QString, QStandardItem*> categories;

        QStandardItem* firstCategory = nullptr;
        QStandardItem* firstItem = nullptr;
        int numCategories = 0;
        int numItems = 0;
        int numItemsAdded = 0;

        for (int unfilteredDataIndex = 0, length = m_unfilteredData->length(); unfilteredDataIndex < length; ++unfilteredDataIndex)
        {
            const SearchTypeFilter& filter = m_unfilteredData->at(unfilteredDataIndex);
            bool addItem = true;

            bool itemMatchesFilter = true;
            bool categoryMatchesFilter = true;

            if (amFiltering)
            {
                itemMatchesFilter = filter.displayName.contains(m_filterString, Qt::CaseSensitivity::CaseInsensitive);
                categoryMatchesFilter = filter.category.contains(m_filterString, Qt::CaseSensitivity::CaseInsensitive);
            }

            if (filterItemOut(unfilteredDataIndex, itemMatchesFilter, categoryMatchesFilter))
            {
                addItem = false;
            }

            QStandardItem* categoryItem = nullptr;
            if (categories.contains(filter.category))
            {
                categoryItem = categories[filter.category];
            }
            else
            {
                if (categoryMatchesFilter || addItem)
                {
                    categoryItem = new QStandardItem(filter.category);
                    categories[filter.category] = categoryItem;
                    m_model->appendRow(categoryItem);
                    categoryItem->setEditable(false);
                    
                    numCategories++;
                    if (!firstCategory)
                    {
                        firstCategory = firstCategory ? firstCategory : categoryItem;
                    }
                }
            }

            // count the item even if we filter it out, so that the estimated height includes what it could be if the filter changes
            numItems++;

            if (!addItem)
            {
                continue;
            }

            numItemsAdded++;

            m_filteredItemIndices.append(unfilteredDataIndex);

            QStandardItem* item = new QStandardItem(filter.displayName);
            item->setData(unfilteredDataIndex);
            item->setEditable(false);

            initItem(item, filter, unfilteredDataIndex);
            
            if (categoryItem)
            {
                categoryItem->appendRow(item);
            }
            else
            {
                m_model->appendRow(item);
            }

            int textWidth = fontMetrics().horizontalAdvance(filter.displayName);
            if (textWidth > m_estimatedTableWidth)
            {
                m_estimatedTableWidth = textWidth;
            }


            if (!firstItem)
            {
                firstItem = item;
            }
        }

        if (numItemsAdded == GetNumFixedItems())
        {
            QStandardItem* item = new QStandardItem(QObject::tr("<i>No result found.</i>"));
            m_model->appendRow(item);
            item->setEditable(false);
            ++numItems;
        }

        // If there is only one category and its name is empty, let put everything at root.
        if (categories.count() == 1 && categories.begin().key().isEmpty())
        {
            m_tree->setRootIndex(categories.begin().value()->index());

            numCategories = 0;
        }

        estimateTableHeight(firstCategory, numCategories, firstItem, numItems);
    }

    void SearchTypeSelector::estimateTableHeight(QStandardItem* firstCategory, int numCategories, QStandardItem* firstItem, int numItems)
    {
        m_tree->expandAll();

        int totalCategoryHeight = 0;
        int totalItemHeight = 0;

        if (firstItem)
        {
            QModelIndex index = m_model->indexFromItem(firstItem);
            int itemHeight = m_tree->fetchRowHeight(index);
            totalItemHeight += (itemHeight * numItems);
        }

        if (firstCategory)
        {
            QModelIndex index = m_model->indexFromItem(firstCategory);
            int categoryHeight = m_tree->fetchRowHeight(index);

            totalCategoryHeight += (categoryHeight * numCategories);
        }

        m_estimatedTableHeight = totalCategoryHeight + totalItemHeight;
    }

    static QPoint searchTypeSelectorPopupPosition(SearchTypeSelector* menu)
    {
        QWidget* parentWidget = menu->parentWidget();

        if (parentWidget)
        {
            return parentWidget->mapToGlobal(parentWidget->rect().bottomLeft());
        }
        else
        {
            return menu->mapToGlobal(QCursor::pos());
        }
    }

    static QRect findScreenGeometry(const QPoint& globalPos)
    {
        QScreen* screen = Utilities::ScreenAtPoint(globalPos);
        return screen->availableGeometry();
    }

    void SearchTypeSelector::maximizeGeometryToFitScreen()
    {
        // Calculate the search layout size (above the tree view)
        int searchLayoutHeight = 0;
        if (m_searchLayout && m_searchField && m_lineEditSearchVisible)
        {
            searchLayoutHeight = m_searchField->height();

            const QMargins margins = m_searchLayout->contentsMargins();
            searchLayoutHeight += margins.top() + margins.bottom();
        }

        int idealHeightGuess = m_estimatedTableHeight + searchLayoutHeight + m_heightEstimatePadding;

        // This repositions and resizes so that the menu:
        // * lies above the parent button if more of it will be visible on the given screen
        // * is not off-screen as a result of adjusting the position and geometry

        int idealHeight = idealHeightGuess;

        QPoint globalPos = searchTypeSelectorPopupPosition(this);

        int parentHeight = parentWidget() ? parentWidget()->height() : 0;
        int globalYAtBottomOfParent = globalPos.y();
        int globalYAtTopOfParent = globalYAtBottomOfParent - parentHeight;

        QRect screenGeometry = findScreenGeometry(globalPos);

        // Decide whether the menu should go up or down from current point.
        // Compare to the mouse position with the remaining height left to store it on either side
        
        int newMaxHeight = screenGeometry.bottom() - globalYAtBottomOfParent;
        int newHeight = idealHeight;

        // We only adjust anything if the idealHeight is too large.
        // Otherwise, we just let it do the default which is to go
        // on the bottom of the screen.
        if (newMaxHeight < idealHeight)
        {
            // If the idealHeight is too large, we check if there's more room on
            // on the top half of the screen
            int topHalfOfScreenMaxHeight = globalYAtTopOfParent - screenGeometry.top();
            if (topHalfOfScreenMaxHeight > newMaxHeight)
            {
                if (topHalfOfScreenMaxHeight < idealHeight)
                {
                    newHeight = topHalfOfScreenMaxHeight;
                    globalPos.setY(screenGeometry.top());
                }
                else
                {
                    globalPos.setY(globalYAtTopOfParent - idealHeight);
                }
            }
            else
            {
                newHeight = newMaxHeight;
                globalPos.setY(globalYAtBottomOfParent);
            }
        }
        else
        {
            globalPos.setY(globalYAtBottomOfParent);
        }

        // Now that we know what sizes to set, set the new policy and the new maximum/minimum
        const QSize menuSize{ m_fixedWidth, newHeight };
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        setMaximumSize(menuSize);
        setMinimumSize(menuSize);

        // Make sure x isn't offscreen too
        if (globalPos.x() < screenGeometry.left())
        {
            globalPos.setX(screenGeometry.left());
        }
        else if ((globalPos.x() + m_fixedWidth) > screenGeometry.right())
        {
            globalPos.setX(screenGeometry.right() - m_fixedWidth);
        }

        move(globalPos);
    }

    void SearchTypeSelector::Setup(const SearchTypeFilterList& searchTypes)
    {
        m_unfilteredData = &searchTypes;

        RepopulateDataModel();

        setFixedWidth(m_estimatedTableWidth + FilterWindowWidthPadding);
    }

    QTreeView* SearchTypeSelector::GetTree()
    {
        return m_tree;
    }

    void SearchTypeSelector::setFixedWidth(int newFixedWidth)
    {
        if (m_fixedWidth != newFixedWidth)
        {
            m_fixedWidth = newFixedWidth;
        }
    }

    void SearchTypeSelector::setHeightEstimatePadding(int newHeightEstimatePadding)
    {
        if (m_heightEstimatePadding != newHeightEstimatePadding)
        {
            m_heightEstimatePadding = newHeightEstimatePadding;
        }
    }

    bool SearchTypeSelector::lineEditSearchVisible() const
    {
        return m_lineEditSearchVisible;
    }

    void SearchTypeSelector::setLineEditSearchVisible(bool visible)
    {
        if (m_lineEditSearchVisible != visible)
        {
            m_searchField->setVisible(visible);

            if (visible)
            {
                m_searchLayout->setContentsMargins(m_searchLayoutMargin, m_searchLayoutMargin, m_searchLayoutMargin, m_searchLayoutMargin);
            }
            else
            {
                m_searchLayout->setContentsMargins(0, 0, 0, 0);
            }

            m_lineEditSearchVisible = visible;
        }
    }

    int SearchTypeSelector::searchLayoutMargin() const
    {
        return m_searchLayoutMargin;
    }
    
    void SearchTypeSelector::setSearchLayoutMargin(int newMargin)
    {
        m_searchLayoutMargin = newMargin;

        if (m_lineEditSearchVisible)
        {
            m_searchLayout->setContentsMargins(m_searchLayoutMargin, m_searchLayoutMargin, m_searchLayoutMargin, m_searchLayoutMargin);
        }
        else
        {
            m_searchLayout->setContentsMargins(0, 0, 0, 0);
        }
    }

    void SearchTypeSelector::FilterTextChanged(const QString& newFilter)
    {
        m_filterString = newFilter;

        RepopulateDataModel();
    }

    FilteredSearchWidget::Config FilteredSearchWidget::loadConfig(QSettings& settings)
    {
        Q_UNUSED(settings);

        Config config = defaultConfig();
        return config;
    }

    FilteredSearchWidget::Config FilteredSearchWidget::defaultConfig()
    {
        Config config;
        return config;
    }

    FilteredSearchWidget::FilteredSearchWidget(QWidget* parent, bool willUseOwnSelector)
        : QFrame(parent)
        , m_ui(new Ui::FilteredSearchWidget)
        , m_flowLayout(new FlowLayout())
        , m_selector(nullptr)
        , m_textFilterFillsWidth(true)
        , m_displayEnabledFilters(true)
    {
        m_ui->setupUi(this);

        // clear the label text by default
        clearLabelText();

        m_ui->filteredLayout->setLayout(m_flowLayout);
        m_ui->filteredParent->hide();
        m_ui->filteredLayout->setContextMenuPolicy(Qt::CustomContextMenu);

        SetTypeFilterVisible(false);
        UpdateTextFilterWidth();

        m_ui->textSearch->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(m_ui->filteredLayout, &QWidget::customContextMenuRequested, this, &FilteredSearchWidget::OnClearFilterContextMenu);
        connect(m_ui->textSearch, &QLineEdit::textChanged, this, &FilteredSearchWidget::OnTextChanged);
        // QLineEdit's clearButton only triggers a textEdited, not a textChanged, so we special case that
        connect(m_ui->textSearch, &QLineEdit::textEdited, this, [this](const QString& newText) {
            if (newText.isEmpty())
            {
                OnTextChanged(newText);
            }
        });
        connect(m_ui->textSearch, &QLineEdit::returnPressed, this, &FilteredSearchWidget::UpdateTextFilter);
        connect(this, &FilteredSearchWidget::textFilterFillsWidthChanged, this, &FilteredSearchWidget::UpdateTextFilterWidth);
        connect(this, &FilteredSearchWidget::TypeFilterChanged, this, [this](const SearchTypeFilterList& activeTypeFilters)
        {
            if (!m_displayEnabledFilters)
            {
                return;
            }

            m_ui->filteredParent->setVisible(!activeTypeFilters.isEmpty());
        });
        connect(m_ui->textSearch, &QWidget::customContextMenuRequested, this, &FilteredSearchWidget::OnSearchContextMenu);

        if (!willUseOwnSelector)
        {
            // have to initialize this after we've called setupUi so that we can make the
            // asset type selector button the parent of the SearchTypeSelector menu. So
            // that it knows how to position itself relative to the button when clicked.
            SetupOwnSelector(new SearchTypeSelector(m_ui->assetTypeSelector));
        }

        connect(&m_inputTimer, &QTimer::timeout, this, &FilteredSearchWidget::UpdateTextFilter);

        m_inputTimer.setInterval(0);
        m_inputTimer.setSingleShot(true);
    }

    FilteredSearchWidget::~FilteredSearchWidget()
    {
        delete m_delegate;
        m_delegate = nullptr;

        delete m_selector;
        m_selector = nullptr;

        delete m_ui;
    }

    void FilteredSearchWidget::SetupOwnSelector(SearchTypeSelector* selector)
    {
        m_selector = selector;
        m_ui->assetTypeSelector->setMenu(m_selector);
        SetupPaintDelegates();

        connect(m_selector, &SearchTypeSelector::aboutToShow, this, [this]() {m_selector->Setup(m_typeFilters); });
        connect(m_selector, &SearchTypeSelector::TypeToggled, this, &FilteredSearchWidget::SetFilterStateByIndex);
    }

    void FilteredSearchWidget::SetTypeFilterVisible(bool visible)
    {
        m_ui->assetTypeSelector->setVisible(visible);
    }

    void FilteredSearchWidget::SetTypeFilters(const SearchTypeFilterList& typeFilters)
    {
        ClearTypeFilter();

        for (const SearchTypeFilter& typeFilter : typeFilters)
        {
            AddTypeFilter(typeFilter);
        }
    }

    void FilteredSearchWidget::AddTypeFilter(const SearchTypeFilter &typeFilter)
    {
        if (!typeFilter.displayName.isEmpty())
        {
            m_typeFilters.append(typeFilter);
            SetFilterStateByIndex(m_typeFilters.length() - 1, typeFilter.enabled);
        }

        SetTypeFilterVisible(true);
    }

    void FilteredSearchWidget::SetTextFilterVisible(bool visible)
    {
        m_ui->textSearch->setVisible(visible);
    }

    QString FilteredSearchWidget::placeholderText() const
    {
        return m_ui->textSearch->placeholderText();
    }

    void FilteredSearchWidget::setPlaceholderText(const QString& placeholderText)
    {
        if (m_ui->textSearch->placeholderText() == placeholderText)
        {
            return;
        }

        m_ui->textSearch->setPlaceholderText(placeholderText);
        emit placeholderTextChanged(placeholderText);
    }

    QString FilteredSearchWidget::textFilter() const
    {
        return m_ui->textSearch->text();
    }

    bool FilteredSearchWidget::hasStringFilter() const
    {
        return !m_ui->textSearch->text().isEmpty();
    }

    void FilteredSearchWidget::SetTextFilter(const QString& textFilter)
    {
        if (m_ui->textSearch->text() == textFilter)
        {
            return;
        }

        {
            // block the signals so that the TextFilterChanged signal doesn't get emitted
            // and we can stop the timer first
            QSignalBlocker blocker(m_ui->textSearch);
            m_ui->textSearch->setText(textFilter);
        }

        UpdateTextFilter();
    }

    void FilteredSearchWidget::ClearTextFilter()
    {
        m_ui->textSearch->clear();
    }

    void FilteredSearchWidget::SetFilterInputInterval(AZStd::chrono::milliseconds milliseconds)
    {
        m_inputTimer.setInterval(static_cast<int>(milliseconds.count()));
    }

    void FilteredSearchWidget::SetFilterState(const QString& category, const QString& displayName, bool enabled)
    {
        int index = FindFilterIndex(category, displayName);
        if (index >= 0)
        {
            SetFilterStateByIndex(index, enabled);
        }
    }

    void  FilteredSearchWidget::SetupPaintDelegates()
    {
        m_delegate = new FilteredSearchItemDelegate(m_selector->GetTree());
        m_selector->GetTree()->setItemDelegate(m_delegate);
        m_delegate->SetSelector(m_selector);
    }

    int FilteredSearchWidget::FindFilterIndex(const QString& category, const QString& displayName) const
    {
        for (int i = 0; i < m_typeFilters.size(); ++i)
        {
            const auto& filter = m_typeFilters[i];
            if (QString::compare(filter.category, category) != 0)
            {
                continue;
            }

            if (QString::compare(filter.displayName, displayName) != 0)
            {
                continue;
            }

            return i;
        }

        return -1;
    }

    void FilteredSearchWidget::ClearTypeFilter()
    {
        for (auto it = m_typeButtons.begin(), end = m_typeButtons.end(); it != end; ++it)
        {
            delete it.value();
        }
        m_typeButtons.clear();

        for (SearchTypeFilter& filter : m_typeFilters)
        {
            filter.enabled = false;
        }

        m_ui->filteredParent->setVisible(false);

        SearchTypeFilterList checkedTypes;
        emit TypeFilterChanged(checkedTypes);
    }

    FilterCriteriaButton* FilteredSearchWidget::createCriteriaButton(const SearchTypeFilter& filter, int filterIndex)
    {
        Q_UNUSED(filterIndex);
        return new FilterCriteriaButton(filter.displayName, this, filter.typeExtraButton, filter.extraIconFilename);
    }

    void FilteredSearchWidget::SetFilterStateByIndex(int index, bool enabled)
    {
        if (index >= m_typeFilters.size())
        {
            return;
        }

        SearchTypeFilter& filter = m_typeFilters[index];

        filter.enabled = enabled;
        auto buttonIt = m_typeButtons.find(index);
        if (enabled && buttonIt == m_typeButtons.end())
        {
            FilterCriteriaButton* button = createCriteriaButton(filter, index);
            connect(button, &FilterCriteriaButton::RequestClose, this, [this, index]() { SetFilterStateByIndex(index, false); });
            connect(button, &FilterCriteriaButton::ExtraButtonClicked, this, [this, index](FilterCriteriaButton::ExtraButtonType type)
            {
                switch (type)
                {
                case FilterCriteriaButton::ExtraButtonType::None:
                    break;
                case FilterCriteriaButton::ExtraButtonType::Locked:
                    //TODO
                    break;
                case FilterCriteriaButton::ExtraButtonType::Unlocked:
                    //TODO
                    break;
                case FilterCriteriaButton::ExtraButtonType::Visible:
                    //TODO
                    break;
                }
            });
            m_flowLayout->addWidget(button);
            m_typeButtons[index] = button;
        }
        else if (!enabled)
        {
            if (buttonIt != m_typeButtons.end())
            {
                delete buttonIt.value();
                m_typeButtons.remove(index);
            }
        }

        emitTypeFilterChanged();
    }

    bool FilteredSearchWidget::textFilterFillsWidth() const
    {
        return m_textFilterFillsWidth;
    }

    void FilteredSearchWidget::setTextFilterFillsWidth(bool fillsWidth)
    {
        if (m_textFilterFillsWidth == fillsWidth)
        {
            return;
        }

        m_textFilterFillsWidth = fillsWidth;
        emit textFilterFillsWidthChanged(m_textFilterFillsWidth);
    }

    void FilteredSearchWidget::clearLabelText()
    {
        setLabelText(QStringLiteral(""));
    }

    void FilteredSearchWidget::setLabelText(const QString& newLabelText)
    {
        if (!newLabelText.isEmpty())
        {
            m_ui->label->setText(newLabelText);

            // Make sure to show the label after setting the text to something valid
            if (!m_ui->label->isVisible())
            {
                m_ui->label->show();
            }
        }
        else
        {
            // Make sure to hide the label after clearing the label, so that the css
            // margins and padding don't apply any longer on an invisible widget
            m_ui->label->clear();
            m_ui->label->hide();
        }
    }

    QString FilteredSearchWidget::labelText() const
    {
        return m_ui->label->text();
    }

    QString FilteredSearchWidget::GetBackgroundColor()
    {
        return BackgroundColor;
    }

    QString FilteredSearchWidget::GetSeparatorColor()
    {
        return SeparatorColor;
    }

    void FilteredSearchWidget::readSettings(QSettings& settings, const QString& widgetName)
    {
        {
            QSignalBlocker blocker(this);
            ConfigHelpers::GroupGuard(&settings, widgetName);
            const auto textFilter = settings.value(g_textFilterKey);
            if (textFilter.isValid())
            {
                SetTextFilter(textFilter.toString());
            }

            const int size = settings.beginReadArray(g_typeFiltersKey);
            for (int i = 0; i < size; ++i)
            {
                settings.setArrayIndex(i);
                const auto category = settings.value(g_categoryKey);
                const auto displayName = settings.value(g_displayNameKey);
                const auto enabled = settings.value(g_enabledKey);
                SetFilterState(category.toString(), displayName.toString(), enabled.toBool());
            }
        }

        emit TextFilterChanged(textFilter());
        emitTypeFilterChanged();
    }

    void FilteredSearchWidget::writeSettings(QSettings& settings, const QString& widgetName)
    {
        ConfigHelpers::GroupGuard(&settings, widgetName);
        settings.setValue(g_textFilterKey, textFilter());

        const int size = m_typeFilters.size();
        settings.beginWriteArray(g_typeFiltersKey, size);
        for (int i = 0; i < size; ++i)
        {
            settings.setArrayIndex(i);
            const auto& filter = m_typeFilters[i];
            settings.setValue(g_categoryKey, filter.category);
            settings.setValue(g_displayNameKey, filter.displayName);
            settings.setValue(g_enabledKey, filter.enabled);
        }
        settings.endArray();
    }

    QToolButton* FilteredSearchWidget::assetTypeSelectorButton() const
    {
        return m_ui->assetTypeSelector;
    }

    void FilteredSearchWidget::emitTypeFilterChanged()
    {
        SearchTypeFilterList checkedTypes;
        for (const SearchTypeFilter& typeFilter : m_typeFilters)
        {
            if (typeFilter.enabled)
            {
                checkedTypes.append(typeFilter);
            }
        }
        emit TypeFilterChanged(checkedTypes);
    }

    QLineEdit* FilteredSearchWidget::filterLineEdit() const
    {
        return m_ui->textSearch;
    }

    QToolButton* FilteredSearchWidget::filterTypePushButton() const
    {
        return m_ui->assetTypeSelector;
    }

    SearchTypeSelector* FilteredSearchWidget::filterTypeSelector() const
    {
        return m_selector;
    }

    const SearchTypeFilterList& FilteredSearchWidget::typeFilters() const
    {
        return m_typeFilters;
    }

    void FilteredSearchWidget::UpdateTextFilterWidth()
    {
        auto sizePolicy = m_ui->textSearch->sizePolicy();
        sizePolicy.setHorizontalStretch(m_textFilterFillsWidth);
        m_ui->textSearch->setSizePolicy(sizePolicy);
    }

    void FilteredSearchWidget::AddWidgetToSearchWidget(QWidget* w)
    {
        m_ui->horizontalLayout_2->addWidget(w);
    }

    void FilteredSearchWidget::OnSearchContextMenu(const QPoint& pos)
    {
        QMenu* menu = m_ui->textSearch->createStandardContextMenu();
        menu->setStyleSheet("background-color: #333333");
        menu->exec(m_ui->textSearch->mapToGlobal(pos));
        delete menu;
    }
    
    void FilteredSearchWidget::OnClearFilterContextMenu(const QPoint& pos)
    {
        QMenu contextMenu(this);

        QAction* action = nullptr;

        action = contextMenu.addAction(QObject::tr("Clear All"));
        QObject::connect(action, &QAction::triggered, this, &FilteredSearchWidget::ClearTypeFilter);

        contextMenu.exec(m_ui->filteredLayout->mapToGlobal(pos));
    }

    void FilteredSearchWidget::SetFilteredParentVisible(bool visible)
    {
        setEnabledFiltersVisible(visible);
    }

    void FilteredSearchWidget::setEnabledFiltersVisible(bool visible)
    {
        if (m_displayEnabledFilters == visible)
        {
            return;
        }

        m_displayEnabledFilters = visible;
        m_ui->filteredParent->setVisible(m_displayEnabledFilters);
    }

    void FilteredSearchWidget::OnTextChanged(const QString& activeTextFilter)
    {
        if (m_inputTimer.interval() == 0 || activeTextFilter.isEmpty())
        {
            UpdateTextFilter();
        }
        else
        {
            m_inputTimer.stop();
            m_inputTimer.start();
        }
    }

    void FilteredSearchWidget::UpdateTextFilter()
    {
        m_inputTimer.stop();
        emit TextFilterChanged(m_ui->textSearch->text());
    }

    bool FilteredSearchWidget::polish(Style* style, QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        auto filteredSearchWidget = qobject_cast<FilteredSearchWidget*>(widget);
        if (!filteredSearchWidget)
        {
            return false;
        }

        LineEdit::applySearchStyle(filteredSearchWidget->m_ui->textSearch);

        return true;
    }

    bool FilteredSearchWidget::unpolish(Style* style, QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        auto filteredSearchWidget = qobject_cast<FilteredSearchWidget*>(widget);
        if (!filteredSearchWidget)
        {
            return false;
        }

        LineEdit::removeSearchStyle(filteredSearchWidget->m_ui->textSearch);

        return true;
    }

    FilteredSearchItemDelegate::FilteredSearchItemDelegate(QWidget* parent) : QStyledItemDelegate(parent)
    {
    }

    void FilteredSearchItemDelegate::PaintRichText(QPainter* painter, QStyleOptionViewItem& opt, QString& text) const
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

    void FilteredSearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
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

    QSize FilteredSearchItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        //correct the vertical space for the separator
        if (opt.text.contains("-------"))
        {
            return QSize(0, 6);
        }
        return QStyledItemDelegate::sizeHint(option, index);
    }
} // namespace AzQtComponents

#include "Components/moc_FilteredSearchWidget.cpp"
