/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemFilterWidget.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QMap>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSignalBlocker>

namespace O3DE::ProjectManager
{
    FilterCategoryWidget::FilterCategoryWidget(const QString& header,
        const QVector<QString>& elementNames,
        const QVector<int>& elementCounts,
        bool showAllLessButton,
        bool collapsed,
        int defaultShowCount,
        QWidget* parent)
        : QWidget(parent)
        , m_defaultShowCount(defaultShowCount)
    {
        AZ_Assert(elementNames.size() == elementCounts.size(), "Number of element names needs to match the counts.");

        QVBoxLayout* vLayout = new QVBoxLayout();
        setLayout(vLayout);

        // Collapse button
        QHBoxLayout* collapseLayout = new QHBoxLayout();
        m_collapseButton = new QPushButton();
        m_collapseButton->setCheckable(true);
        m_collapseButton->setChecked(collapsed);
        m_collapseButton->setFlat(true);
        m_collapseButton->setFocusPolicy(Qt::NoFocus);
        m_collapseButton->setFixedWidth(s_collapseButtonSize);
        connect(m_collapseButton, &QPushButton::clicked, this, [=]()
            {
                UpdateCollapseState();
            });
        collapseLayout->addWidget(m_collapseButton);

        // Category title
        QLabel* headerLabel = new QLabel(header);
        headerLabel->setObjectName("GemCatalogFilterCategoryTitle");
        collapseLayout->addWidget(headerLabel);
        vLayout->addLayout(collapseLayout);

        vLayout->addSpacing(5);

        // Everything in the main widget will be collapsed/uncollapsed
        {
            m_mainWidget = new QWidget();
            vLayout->addWidget(m_mainWidget);

            QVBoxLayout* mainLayout = new QVBoxLayout();
            mainLayout->setMargin(0);
            mainLayout->setAlignment(Qt::AlignTop);
            m_mainWidget->setLayout(mainLayout);

            // Elements
            m_buttonGroup = new QButtonGroup();
            m_buttonGroup->setExclusive(false);
            for (int i = 0; i < elementNames.size(); ++i)
            {
                QWidget* elementWidget = new QWidget();
                QHBoxLayout* elementLayout = new QHBoxLayout();
                elementLayout->setMargin(0);
                elementWidget->setLayout(elementLayout);

                QCheckBox* checkbox = new QCheckBox(elementNames[i]);
                checkbox->setStyleSheet("font-size: 12px;");
                m_buttonGroup->addButton(checkbox);
                elementLayout->addWidget(checkbox);

                elementLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

                QLabel* countLabel = new QLabel(QString::number(elementCounts[i]));
                countLabel->setStyleSheet("font-size: 12px; background-color: #333333; border-radius: 3px; color: #94D2FF;");
                elementLayout->addWidget(countLabel);

                m_elementWidgets.push_back(elementWidget);
                mainLayout->addWidget(elementWidget);
            }

            // See more / less
            if (showAllLessButton)
            {
                m_seeAllLessLabel = new LinkLabel();
                connect(m_seeAllLessLabel, &LinkLabel::clicked, this, [=]()
                    {
                        m_seeAll = !m_seeAll;
                        UpdateSeeMoreLess();
                    });
                mainLayout->addWidget(m_seeAllLessLabel);
            }
            else
            {
                mainLayout->addSpacing(5);
            }
        }

        vLayout->addSpacing(5);

        // Separating line
        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setStyleSheet("color: #666666;");
        vLayout->addWidget(hLine);

        UpdateCollapseState();
        UpdateSeeMoreLess();
    }

    void FilterCategoryWidget::UpdateCollapseState()
    {
        if (m_collapseButton->isChecked())
        {
            m_collapseButton->setIcon(QIcon(":/ArrowDownLine.svg"));
            m_mainWidget->hide();
        }
        else
        {
            m_collapseButton->setIcon(QIcon(":/ArrowUpLine.svg"));
            m_mainWidget->show();
        }
    }

    void FilterCategoryWidget::UpdateSeeMoreLess()
    {
        if (!m_seeAllLessLabel)
        {
            return;
        }

        if (m_elementWidgets.isEmpty())
        {
            m_seeAllLessLabel->hide();
            return;
        }
        else
        {
            m_seeAllLessLabel->show();
        }

        if (!m_seeAll)
        {
            m_seeAllLessLabel->setText("See all");
        }
        else
        {
            m_seeAllLessLabel->setText("See less");
        }

        int showCount = m_seeAll ? m_elementWidgets.size() : m_defaultShowCount;
        showCount = AZ::GetMin(showCount, m_elementWidgets.size());
        for (int i = 0; i < showCount; ++i)
        {
            m_elementWidgets[i]->show();
        }
        for (int i = showCount; i < m_elementWidgets.size(); ++i)
        {
            m_elementWidgets[i]->hide();
        }
    }

    QButtonGroup* FilterCategoryWidget::GetButtonGroup()
    {
        return m_buttonGroup;
    }

    bool FilterCategoryWidget::IsCollapsed()
    {
        return m_collapseButton->isChecked();
    }

    GemFilterWidget::GemFilterWidget(GemSortFilterProxyModel* filterProxyModel, QWidget* parent)
        : QScrollArea(parent)
        , m_filterProxyModel(filterProxyModel)
    {
        setObjectName("GemCatalogFilterWidget");

        m_gemModel = m_filterProxyModel->GetSourceModel();

        setWidgetResizable(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        QWidget* mainWidget = new QWidget();
        setWidget(mainWidget);

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setAlignment(Qt::AlignTop);
        mainWidget->setLayout(mainLayout);

        QLabel* filterByLabel = new QLabel("Filter by");
        filterByLabel->setStyleSheet("font-size: 16px;");
        mainLayout->addWidget(filterByLabel);

        QWidget* filterSection = new QWidget(this);
        mainLayout->addWidget(filterSection);

        m_filterLayout = new QVBoxLayout();
        m_filterLayout->setAlignment(Qt::AlignTop);
        m_filterLayout->setContentsMargins(0, 0, 0, 0);
        filterSection->setLayout(m_filterLayout);

        ResetAllFilters();
    }

    void GemFilterWidget::ResetAllFilters()
    {
        ResetGemStatusFilter();
        ResetGemOriginFilter();
        ResetTypeFilter();
        ResetFeatureFilter();
    }

    void GemFilterWidget::ResetFilterWidget(
        FilterCategoryWidget*& filterPtr,
        const QString& filterName,
        const QVector<QString>& elementNames,
        const QVector<int>& elementCounts,
        int defaultShowCount)
    {
        bool wasCollapsed = false;
        if (filterPtr)
        {
            wasCollapsed = filterPtr->IsCollapsed();
        }

        FilterCategoryWidget* filterWidget = new FilterCategoryWidget(
            filterName, elementNames, elementCounts, /*showAllLessButton=*/defaultShowCount != 4, /*collapsed*/ wasCollapsed,
            /*defaultShowCount=*/defaultShowCount);
        if (filterPtr)
        {
            m_filterLayout->replaceWidget(filterPtr, filterWidget);
        }
        else
        {
            m_filterLayout->addWidget(filterWidget);
        }

        filterPtr->deleteLater();
        filterPtr = filterWidget;
    }

    template<typename filterType, typename filterFlagsType>
    void GemFilterWidget::ResetSimpleOrFilter(
        FilterCategoryWidget*& filterPtr,
        const QString& filterName,
        int numFilterElements,
        bool (*filterMatcher)(GemModel*, filterType, int),
        QString (*typeStringGetter)(filterType),
        filterFlagsType (GemSortFilterProxyModel::*filterFlagsGetter)() const,
        void (GemSortFilterProxyModel::*filterFlagsSetter)(const filterFlagsType&))
    {
        QVector<QString> elementNames;
        QVector<int> elementCounts;
        const int numGems = m_gemModel->rowCount();
        for (int filterIndex = 0; filterIndex < numFilterElements; ++filterIndex)
        {
            const filterType gemFilterToBeCounted = static_cast<filterType>(1 << filterIndex);

            int gemFilterCount = 0;
            for (int gemIndex = 0; gemIndex < numGems; ++gemIndex)
            {
                // If filter matches increment filter count
                gemFilterCount += filterMatcher(m_gemModel, gemFilterToBeCounted, gemIndex);
            }
            elementNames.push_back(typeStringGetter(gemFilterToBeCounted));
            elementCounts.push_back(gemFilterCount);
        }

        // Replace existing filter and delete old one
        ResetFilterWidget(filterPtr, filterName, elementNames, elementCounts);

        const QList<QAbstractButton*> buttons = filterPtr->GetButtonGroup()->buttons();
        for (int i = 0; i < buttons.size(); ++i)
        {
            const filterType gemFilter = static_cast<filterType>(1 << i);
            QAbstractButton* button = buttons[i];

            connect(
                button, &QAbstractButton::toggled, this,
                [=](bool checked)
                {
                    filterFlagsType gemFilters = (m_filterProxyModel->*filterFlagsGetter)();
                    if (checked)
                    {
                        gemFilters |= gemFilter;
                    }
                    else
                    {
                        gemFilters &= ~gemFilter;
                    }
                    (m_filterProxyModel->*filterFlagsSetter)(gemFilters);
                });
        }
    }

    void GemFilterWidget::ResetGemStatusFilter()
    {
        QVector<QString> elementNames;
        QVector<int> elementCounts;
        const int totalGems = m_gemModel->rowCount();
        const int selectedGemTotal = m_gemModel->GatherGemsToBeAdded(/*includeDependencies=*/true).size();
        const int unselectedGemTotal = m_gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/true).size();
        const int enabledGemTotal = m_gemModel->TotalAddedGems(/*includeDependencies=*/true);

        elementNames.push_back(GemSortFilterProxyModel::GetGemSelectedString(GemSortFilterProxyModel::GemSelected::Selected));
        elementCounts.push_back(selectedGemTotal);

        elementNames.push_back(GemSortFilterProxyModel::GetGemSelectedString(GemSortFilterProxyModel::GemSelected::Unselected));
        elementCounts.push_back(unselectedGemTotal);

        elementNames.push_back(GemSortFilterProxyModel::GetGemActiveString(GemSortFilterProxyModel::GemActive::Active));
        elementCounts.push_back(enabledGemTotal);

        elementNames.push_back(GemSortFilterProxyModel::GetGemActiveString(GemSortFilterProxyModel::GemActive::Inactive));
        elementCounts.push_back(totalGems - enabledGemTotal);

        ResetFilterWidget(m_statusFilter, "Status", elementNames, elementCounts);

        const QList<QAbstractButton*> buttons = m_statusFilter->GetButtonGroup()->buttons();

        QAbstractButton* selectedButton = buttons[0];
        QAbstractButton* unselectedButton = buttons[1];
        selectedButton->setChecked(m_filterProxyModel->GetGemSelected() == GemSortFilterProxyModel::GemSelected::Selected);
        unselectedButton->setChecked(m_filterProxyModel->GetGemSelected() == GemSortFilterProxyModel::GemSelected::Unselected);

        auto updateGemSelection = [=]([[maybe_unused]] bool checked)
        {
            if (!unselectedButton->isChecked() && selectedButton->isChecked())
            {
                m_filterProxyModel->SetGemSelected(GemSortFilterProxyModel::GemSelected::Selected);
            }
            else if (unselectedButton->isChecked() && !selectedButton->isChecked())
            {
                m_filterProxyModel->SetGemSelected(GemSortFilterProxyModel::GemSelected::Unselected);
            }
            else
            {
                if (unselectedButton->isChecked() && selectedButton->isChecked())
                {
                    m_filterProxyModel->SetGemSelected(GemSortFilterProxyModel::GemSelected::Both);
                }
                else
                {
                    m_filterProxyModel->SetGemSelected(GemSortFilterProxyModel::GemSelected::NoFilter);
                }
            }
        };
        connect(unselectedButton, &QAbstractButton::toggled, this, updateGemSelection);
        connect(selectedButton, &QAbstractButton::toggled, this, updateGemSelection);

        QAbstractButton* activeButton = buttons[2];
        QAbstractButton* inactiveButton = buttons[3];
        activeButton->setChecked(m_filterProxyModel->GetGemActive() == GemSortFilterProxyModel::GemActive::Active);
        inactiveButton->setChecked(m_filterProxyModel->GetGemActive() == GemSortFilterProxyModel::GemActive::Inactive);

        auto updateGemActive = [=]([[maybe_unused]] bool checked)
        {
            if (!inactiveButton->isChecked() && activeButton->isChecked())
            {
                m_filterProxyModel->SetGemActive(GemSortFilterProxyModel::GemActive::Active);
            }
            else if (inactiveButton->isChecked() && !activeButton->isChecked())
            {
                m_filterProxyModel->SetGemActive(GemSortFilterProxyModel::GemActive::Inactive);
            }
            else
            {
                m_filterProxyModel->SetGemActive(GemSortFilterProxyModel::GemActive::NoFilter);
            }
        };
        connect(inactiveButton, &QAbstractButton::toggled, this, updateGemActive);
        connect(activeButton, &QAbstractButton::toggled, this, updateGemActive);
    }

    void GemFilterWidget::ResetGemOriginFilter()
    {
        ResetSimpleOrFilter<GemInfo::GemOrigin, GemInfo::GemOrigins>
        (
            m_originFilter, "Provider", GemInfo::NumGemOrigins,
            [](GemModel* gemModel, GemInfo::GemOrigin origin, int gemIndex)
            {
                return origin == gemModel->GetGemOrigin(gemModel->index(gemIndex, 0)); 
            },
            &GemInfo::GetGemOriginString, &GemSortFilterProxyModel::GetGemOrigins, &GemSortFilterProxyModel::SetGemOrigins
        );
    }

    void GemFilterWidget::ResetTypeFilter()
    {
        ResetSimpleOrFilter<GemInfo::Type, GemInfo::Types>(
            m_typeFilter, "Type", GemInfo::NumTypes,
            [](GemModel* gemModel, GemInfo::Type type, int gemIndex)
            {
                return static_cast<bool>(type & gemModel->GetTypes(gemModel->index(gemIndex, 0)));
            },
            &GemInfo::GetTypeString, &GemSortFilterProxyModel::GetTypes, &GemSortFilterProxyModel::SetTypes);
    }

    void GemFilterWidget::ResetPlatformFilter()
    {
        ResetSimpleOrFilter<GemInfo::Platform, GemInfo::Platforms>(
            m_platformFilter, "Supported Platforms", GemInfo::NumPlatforms,
            [](GemModel* gemModel, GemInfo::Platform platform, int gemIndex)
            {
                return static_cast<bool>(platform & gemModel->GetPlatforms(gemModel->index(gemIndex, 0)));
            },
            &GemInfo::GetPlatformString, &GemSortFilterProxyModel::GetPlatforms, &GemSortFilterProxyModel::SetPlatforms);
    }

    void GemFilterWidget::ResetFeatureFilter()
    {
        // Alphabetically sorted, unique features and their number of occurrences in the gem database.
        QMap<QString, int> uniqueFeatureCounts;
        const int numGems = m_gemModel->rowCount();
        for (int gemIndex = 0; gemIndex < numGems; ++gemIndex)
        {
            const QStringList features = m_gemModel->GetFeatures(m_gemModel->index(gemIndex, 0));
            for (const QString& feature : features)
            {
                if (!uniqueFeatureCounts.contains(feature))
                {
                    uniqueFeatureCounts.insert(feature, 1);
                }
                else
                {
                    int& featureeCount = uniqueFeatureCounts[feature];
                    featureeCount++;
                }
            }
        }

        QVector<QString> elementNames;
        QVector<int> elementCounts;
        for (auto iterator = uniqueFeatureCounts.begin(); iterator != uniqueFeatureCounts.end(); iterator++)
        {
            elementNames.push_back(iterator.key());
            elementCounts.push_back(iterator.value());
        }

        ResetFilterWidget(m_featureFilter, "Features", elementNames, elementCounts, /*defaultShowCount=*/5);

        for (QMetaObject::Connection& connection : m_featureTagConnections)
        {
            disconnect(connection);
        }
        m_featureTagConnections.clear();

        const QList<QAbstractButton*> buttons = m_featureFilter->GetButtonGroup()->buttons();
        for (int i = 0; i < buttons.size(); ++i)
        {
            const QString& feature = elementNames[i];
            QAbstractButton* button = buttons[i];

            // Adjust the proxy model and enable the clicked feature used for filtering.
            connect(button, &QAbstractButton::toggled, this, [=](bool checked)
                {
                    QSet<QString> features = m_filterProxyModel->GetFeatures();
                    if (checked)
                    {
                        features.insert(feature);
                    }
                    else
                    {
                        features.remove(feature);
                    }
                    m_filterProxyModel->SetFeatures(features);
                });

            // Sync the UI state with the proxy model filtering.
            m_featureTagConnections.push_back(connect(m_filterProxyModel, &GemSortFilterProxyModel::OnInvalidated, this, [=]
                {
                    const QSet<QString>& filteredFeatureTags = m_filterProxyModel->GetFeatures();
                    const bool isChecked = filteredFeatureTags.contains(button->text());
                    QSignalBlocker signalsBlocker(button);
                    button->setChecked(isChecked);
                }));
        }
    }
} // namespace O3DE::ProjectManager
