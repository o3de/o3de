/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemFilterWidget.h>

#include <AzCore/Math/MathUtils.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QMap>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>

namespace O3DE::ProjectManager
{
    FilterCategoryWidget::FilterCategoryWidget(
        const QString& header, bool showAllLessButton, bool collapsed, int defaultShowCount, QWidget* parent)
        : QWidget(parent)
        , m_defaultShowCount(defaultShowCount)
    {
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
        connect(m_collapseButton, &QPushButton::clicked, this, &FilterCategoryWidget::UpdateCollapseState);
        collapseLayout->addWidget(m_collapseButton);

        // Category title
        QLabel* headerLabel = new QLabel(header);
        headerLabel->setObjectName("GemCatalogFilterCategoryTitle");
        collapseLayout->addWidget(headerLabel);
        vLayout->addLayout(collapseLayout);

        vLayout->addSpacing(5);

        // Everything in the main widget can be collapsed/uncollapsed
        {
            m_mainWidget = new QWidget();
            vLayout->addWidget(m_mainWidget);

            QVBoxLayout* mainLayout = new QVBoxLayout();
            mainLayout->setMargin(0);
            mainLayout->setAlignment(Qt::AlignTop);
            m_mainWidget->setLayout(mainLayout);

            // Elements
            QVBoxLayout* elementsLayout = new QVBoxLayout();
            m_elementsWidget = new QWidget();
            elementsLayout->setMargin(0);
            m_elementsWidget->setLayout(elementsLayout);
            mainLayout->addWidget(m_elementsWidget);

            m_buttonGroup = new QButtonGroup();
            m_buttonGroup->setExclusive(false);

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
        hLine->setObjectName("horizontalSeparatingLine");
        vLayout->addWidget(hLine);

        UpdateCollapseState();
        UpdateSeeMoreLess();
    }

    void FilterCategoryWidget::SetElement(int index, const QString& name, int count)
    {
        const auto& elements = GetElements();
        if (index >= elements.size())
        {
            QWidget* elementWidget = new QWidget();
            QHBoxLayout* elementLayout = new QHBoxLayout();
            elementLayout->setMargin(0);
            elementWidget->setLayout(elementLayout);

            QCheckBox* checkbox = new QCheckBox(name);
            m_buttonGroup->addButton(checkbox);
            elementLayout->addWidget(checkbox);

            elementLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

            QLabel* countLabel = new QLabel(QString::number(count));
            countLabel->setObjectName("CountLabel");
            elementLayout->addWidget(countLabel);

            m_elementsWidget->layout()->addWidget(elementWidget);
        }
        else
        {
            QWidget* elementWidget = elements.at(index);

            QCheckBox* checkbox = elementWidget->findChild<QCheckBox*>();
            if (checkbox)
            {
                checkbox->setText(name);
            }

            QLabel* label = elementWidget->findChild<QLabel*>("CountLabel");
            if (label)
            {
                label->setText(QString::number(count));
            }
        }
    }

    int FilterCategoryWidget::RemoveUnusedElements(uint32_t usedCount)
    {
        const auto& elements = GetElements();
        const int numToDelete = elements.size() - usedCount;
        if (numToDelete > 0)
        {
            qDeleteAll(elements.cend() - numToDelete, elements.cend());
        }
        return numToDelete;
    }

    void FilterCategoryWidget::SetElements(const QMap<QString, int>& elementNamesAndCounts)
    {
        int i = 0;
        const int numChildren = GetElements().size();
        for (auto iter = elementNamesAndCounts.begin(); iter != elementNamesAndCounts.end(); iter++)
        {
            SetElement(i, iter.key(), iter.value());
            i++;
        }
        RemoveUnusedElements(elementNamesAndCounts.size());

        // if the number of elements changed we need to update the collapsed state
        if(numChildren != GetElements().size())
        {
            UpdateCollapseState();
            UpdateSeeMoreLess();
        }
    }

    void FilterCategoryWidget::SetElements(const QVector<QString>& elementNames, const QVector<int>& elementCounts)
    {
        const int numChildren = GetElements().size();
        for (int i = 0; i < elementNames.size(); ++i)
        {
            SetElement(i, elementNames[i], elementCounts[i]);
        }
        RemoveUnusedElements(elementNames.size());

        // if the number of elements changed we need to update the collapsed state
        if(numChildren != GetElements().size())
        {
            UpdateCollapseState();
            UpdateSeeMoreLess();
        }
    }

    QWidgetList FilterCategoryWidget::GetElements() const
    {
        return m_elementsWidget->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly);
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

        const auto& elements = GetElements();
        if (elements.isEmpty())
        {
            m_seeAllLessLabel->hide();
            return;
        }

        m_seeAllLessLabel->show();
        m_seeAllLessLabel->setText(m_seeAll ? tr("See less") : tr("See all"));

        int showCount = m_seeAll ? elements.size() : m_defaultShowCount;
        showCount = AZ::GetMin(showCount, elements.size());
        for (int i = 0; i < showCount; ++i)
        {
            elements[i]->show();
        }

        for (int i = showCount; i < elements.size(); ++i)
        {
            elements[i]->hide();
        }
    }

    QButtonGroup* FilterCategoryWidget::GetButtonGroup()
    {
        return m_buttonGroup;
    }

    OrFilterCategoryWidget::OrFilterCategoryWidget(const QString& header, int numFilterElements, GemModel* gemModel)
        : FilterCategoryWidget(header, false, false, numFilterElements)
        , m_numFilterElements(numFilterElements)
        , m_gemModel(gemModel)
    {
        connect( GetButtonGroup(), QOverload<QAbstractButton*, bool>::of(&QButtonGroup::buttonToggled), this,
            [this](QAbstractButton* button, bool checked)
            {
                const int buttonId = GetButtonGroup()->id(button);
                AZ_Assert(buttonId != -1, "Invalid button id");

                // generated button ids are negative starting at -2
                // https://doc.qt.io/qt-5/qbuttongroup.html#addButton
                const int flag = 1 << (-buttonId - 2);
                emit FilterToggled(flag, checked);
            });
    }

    void OrFilterCategoryWidget::UpdateFilter(
        bool(*filterMatch)(const GemInfo& gemInfo, int filterIndex), 
        QString(*filterLabel)(int filterIndex)
        )
    {
        QVector<QString> elementNames;
        QVector<int> elementCounts;
        const int numGems = m_gemModel->rowCount();
        for (int filterIndex = 0; filterIndex < m_numFilterElements; ++filterIndex)
        {
            int gemFilterCount = 0;
            for (int gemIndex = 0; gemIndex < numGems; ++gemIndex)
            {
                // If filter matches increment filter count
                const GemInfo& gemInfo = m_gemModel->GetGemInfo(m_gemModel->index(gemIndex, 0));
                gemFilterCount += filterMatch(gemInfo, filterIndex);
            }
            elementNames.push_back(filterLabel(filterIndex));
            elementCounts.push_back(gemFilterCount);
        }

        SetElements(elementNames, elementCounts);
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
        filterByLabel->setObjectName("FilterByLabel");
        mainLayout->addWidget(filterByLabel);

        QWidget* filterSection = new QWidget(this);
        mainLayout->addWidget(filterSection);

        QVBoxLayout* filterLayout = new QVBoxLayout();
        filterLayout->setAlignment(Qt::AlignTop);
        filterLayout->setContentsMargins(0, 0, 0, 0);
        filterSection->setLayout(filterLayout);

        m_statusFilter = new FilterCategoryWidget("Status");
        connect(m_statusFilter->GetButtonGroup(), QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled), this, &GemFilterWidget::OnStatusFilterToggled);

        m_versionsFilter = new FilterCategoryWidget("Versions");
        connect(m_versionsFilter->GetButtonGroup(), QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled), this, &GemFilterWidget::OnUpdateFilterToggled);

        m_featureFilter = new FilterCategoryWidget("Feature", /*showAllLessButton=*/true, /*collapsed*/false, /*defaultShowCount=*/5);
        connect(m_featureFilter->GetButtonGroup(), QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled), this, &GemFilterWidget::OnFeatureFilterToggled);

        m_platformFilter = new OrFilterCategoryWidget("Platform", GemInfo::NumPlatforms, m_gemModel);
        connect(m_platformFilter, &OrFilterCategoryWidget::FilterToggled, m_filterProxyModel, &GemSortFilterProxyModel::SetPlatformFilterFlag);

        m_originFilter = new OrFilterCategoryWidget("Provider", GemInfo::NumGemOrigins, m_gemModel);
        connect(m_originFilter, &OrFilterCategoryWidget::FilterToggled, m_filterProxyModel, &GemSortFilterProxyModel::SetOriginFilterFlag);

        m_typeFilter = new OrFilterCategoryWidget("Type", GemInfo::NumTypes, m_gemModel);
        connect(m_typeFilter, &OrFilterCategoryWidget::FilterToggled, m_filterProxyModel, &GemSortFilterProxyModel::SetTypeFilterFlag);

        // add filters in the order they appear
        filterLayout->addWidget(m_statusFilter);
        filterLayout->addWidget(m_versionsFilter);
        filterLayout->addWidget(m_originFilter);
        filterLayout->addWidget(m_typeFilter);
        filterLayout->addWidget(m_featureFilter);
        filterLayout->addWidget(m_platformFilter);

        UpdateAllFilters();

        connect(m_filterProxyModel, &GemSortFilterProxyModel::OnInvalidated, this, &GemFilterWidget::OnFilterProxyInvalidated);
    }

    void ResetButtonCheckBoxes(FilterCategoryWidget* widget)
    {
        for (const auto& button : widget->GetButtonGroup()->buttons())
        {
            button->setChecked(button->property("selected_by_default").isValid());
        }
    }

    void GemFilterWidget::UpdateAllFilters(bool resetCheckBoxes)
    {
        UpdateGemStatusFilter();
        UpdateVersionsFilter();
        UpdateGemOriginFilter();
        UpdateTypeFilter();
        UpdateFeatureFilter();
        UpdatePlatformFilter();

        if (resetCheckBoxes)
        {
            ResetButtonCheckBoxes(m_statusFilter);
            ResetButtonCheckBoxes(m_versionsFilter);
            ResetButtonCheckBoxes(m_originFilter);
            ResetButtonCheckBoxes(m_typeFilter);
            ResetButtonCheckBoxes(m_featureFilter);
            ResetButtonCheckBoxes(m_platformFilter);
        }
    }

    void GemFilterWidget::UpdateVersionsFilter()
    {
        int numGemsWithUpdates = 0;
        int numCompatibleGems = 0;

        // check the state of the compatible filter to see if we should be
        // including updates for incompatible versions
        bool compatibleUpdatesOnly = true; // on by default
        QList<QAbstractButton*> buttons = m_versionsFilter->GetButtonGroup()->buttons();
        if (buttons.count() >= 2)
        {
            const QAbstractButton* compatibleButton = buttons[1];
            compatibleUpdatesOnly = compatibleButton->isChecked();
        }

        for (int i = 0; i < m_gemModel->rowCount(); ++i)
        {
            numGemsWithUpdates += GemModel::HasUpdates(m_gemModel->index(i, 0), compatibleUpdatesOnly) ? 1 : 0;
            numCompatibleGems += GemModel::IsCompatible(m_gemModel->index(i, 0)) ? 1 : 0;
        }

        m_versionsFilter->SetElements({ "Update Available", "Compatible" }, { numGemsWithUpdates, numCompatibleGems });

        if (buttons.isEmpty())
        {
            // buttons were just created so make sure to set the selected_by_default property
            // for the checkboxes we want to be on by default
            buttons = m_versionsFilter->GetButtonGroup()->buttons();
            QAbstractButton* compatibleButton = buttons[1];
            compatibleButton->setProperty("selected_by_default", true);
        }
    }

    void GemFilterWidget::OnUpdateFilterToggled(QAbstractButton* button, bool checked)
    {
        const QList<QAbstractButton*> buttons = m_versionsFilter->GetButtonGroup()->buttons();

        if(button == buttons[0])
        {
            m_filterProxyModel->SetUpdateAvailable(checked);
        }

        if(button == buttons[1])
        {
            if(checked)
            {
                // have the gem model update the current gems with compatible
                // versions in case the user was looking at incompatible gems
                // and compatible gems exist
                m_gemModel->ShowCompatibleGems();
            }

            // when the compatibility filter is changed we need to update the
            // counts for the "Updates Available"
            UpdateVersionsFilter();

            m_filterProxyModel->SetCompatibleFilterFlag(checked);
        }
    }

    void GemFilterWidget::OnStatusFilterToggled(QAbstractButton* button, bool checked)
    {
        const QList<QAbstractButton*> buttons = m_statusFilter->GetButtonGroup()->buttons();
        QAbstractButton* selectedButton = buttons[0];
        QAbstractButton* unselectedButton = buttons[1];
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

        QAbstractButton* activeButton = buttons[2];
        QAbstractButton* inactiveButton = buttons[3];
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

        // Missing
        if (button == buttons[4])
        {
            m_filterProxyModel->SetGemMissing(checked);
        }
    }

    void GemFilterWidget::UpdateGemStatusFilter()
    {
        QVector<QString> elementNames;
        QVector<int> elementCounts;
        const int totalGems = m_gemModel->rowCount();
        const int selectedGemTotal = m_gemModel->GatherGemsToBeAdded(/*includeDependencies=*/true).size();
        const int unselectedGemTotal = m_gemModel->GatherGemsToBeRemoved(/*includeDependencies=*/true).size();
        const int enabledGemTotal = m_gemModel->TotalAddedGems(/*includeDependencies=*/true);

        if (selectedGemTotal == 0 && enabledGemTotal == 0 && unselectedGemTotal == 0 && totalGems > 0)
        {
            // no gems are selected, unselected or enabled  
            m_statusFilter->setVisible(false);
            return;
        }

        m_statusFilter->setVisible(true);

        elementNames.push_back(GemSortFilterProxyModel::GetGemSelectedString(GemSortFilterProxyModel::GemSelected::Selected));
        elementCounts.push_back(selectedGemTotal);

        elementNames.push_back(GemSortFilterProxyModel::GetGemSelectedString(GemSortFilterProxyModel::GemSelected::Unselected));
        elementCounts.push_back(unselectedGemTotal);

        elementNames.push_back(GemSortFilterProxyModel::GetGemActiveString(GemSortFilterProxyModel::GemActive::Active));
        elementCounts.push_back(enabledGemTotal);

        elementNames.push_back(GemSortFilterProxyModel::GetGemActiveString(GemSortFilterProxyModel::GemActive::Inactive));
        elementCounts.push_back(totalGems - enabledGemTotal);

        elementNames.push_back(tr("Missing"));
        int numMissingGems = 0;
        for (int i = 0; i < m_gemModel->rowCount(); ++i)
        {
            numMissingGems += GemModel::IsAddedMissing(m_gemModel->index(i, 0)) ? 1 : 0;
        }
        elementCounts.push_back(numMissingGems);

        m_statusFilter->SetElements(elementNames, elementCounts);

        const QList<QAbstractButton*> buttons = m_statusFilter->GetButtonGroup()->buttons();
        QAbstractButton* selectedButton = buttons[0];
        QAbstractButton* unselectedButton = buttons[1];
        selectedButton->setChecked(m_filterProxyModel->GetGemSelected() == GemSortFilterProxyModel::GemSelected::Selected);
        unselectedButton->setChecked(m_filterProxyModel->GetGemSelected() == GemSortFilterProxyModel::GemSelected::Unselected);

        QAbstractButton* activeButton = buttons[2];
        QAbstractButton* inactiveButton = buttons[3];
        activeButton->setChecked(m_filterProxyModel->GetGemActive() == GemSortFilterProxyModel::GemActive::Active);
        inactiveButton->setChecked(m_filterProxyModel->GetGemActive() == GemSortFilterProxyModel::GemActive::Inactive);

        QAbstractButton* missingButton = buttons[4];
        missingButton->setChecked(m_filterProxyModel->GetMissingActive());
    }

    void GemFilterWidget::UpdateGemOriginFilter()
    {
        m_originFilter->UpdateFilter(
            // filterMatch
            [](const GemInfo& gemInfo, int filterIndex)
            {
                return gemInfo.m_gemOrigin == static_cast<GemInfo::GemOrigin>(1 << filterIndex);
            },
            // filterLabel
            [](int filterIndex)
            {
                return GemInfo::GetGemOriginString(static_cast<GemInfo::GemOrigin>(1 << filterIndex));
            });
    }

    void GemFilterWidget::UpdateTypeFilter()
    {
        m_typeFilter->UpdateFilter(
            // filterMatch
            [](const GemInfo& gemInfo, int filterIndex)
            {
                return (gemInfo.m_types & static_cast<GemInfo::Type>(1 << filterIndex)) != 0;
            },
            // filterLabel
            [](int filterIndex)
            {
                return GemInfo::GetTypeString(static_cast<GemInfo::Type>(1 << filterIndex));
            });
    }

    void GemFilterWidget::UpdatePlatformFilter()
    {
        m_platformFilter->UpdateFilter(
            // filterMatch
            [](const GemInfo& gemInfo, int filterIndex)
            {
                return (gemInfo.m_platforms & static_cast<GemInfo::Platform>(1 << filterIndex)) != 0;
            },
            // filterLabel
            [](int filterIndex)
            {
                return GemInfo::GetPlatformString(static_cast<GemInfo::Platform>(1 << filterIndex));
            });
    }

    void GemFilterWidget::OnFeatureFilterToggled(QAbstractButton* button, bool checked)
    {
        QString feature = button->text();
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
    }

    void GemFilterWidget::OnFilterProxyInvalidated()
    {
        if (!m_featureFilter || !m_filterProxyModel)
        {
            return;
        }

        const QSet<QString>& filteredFeatureTags = m_filterProxyModel->GetFeatures();
        const QList<QAbstractButton*> buttons = m_featureFilter->GetButtonGroup()->buttons();
        for (auto button : buttons)
        {
            const bool isChecked = filteredFeatureTags.contains(button->text());
            QSignalBlocker signalsBlocker(button);
            button->setChecked(isChecked);
        }
    }

    void GemFilterWidget::UpdateFeatureFilter()
    {
        // Alphabetically sorted, unique features and their number of occurrences in the gem database.
        QMap<QString, int> uniqueFeatureCounts;
        const int numGems = m_gemModel->rowCount();
        for (int gemIndex = 0; gemIndex < numGems; ++gemIndex)
        {
            const QStringList& features = m_gemModel->GetGemInfo(m_gemModel->index(gemIndex, 0)).m_features;
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

        m_featureFilter->SetElements(uniqueFeatureCounts);
    }
} // namespace O3DE::ProjectManager
