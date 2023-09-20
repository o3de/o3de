/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QTextFormat::d': class 'QSharedDataPointer<QTextFormatPrivate>' needs to have dll-interface to be used by clients of class 'QTextFormat'
#include <QLineEdit>
#include <QToolButton>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        namespace
        {
            AzQtComponents::SearchTypeFilterList buildTypesFilterList()
            {
                AzQtComponents::SearchTypeFilterList filters;

                EBusAggregateUniqueResults<QString> groups;
                AZ::AssetTypeInfoBus::BroadcastResult(groups, &AZ::AssetTypeInfo::GetGroup);

                // Group "Other" should be in the end of the list, and "Hidden" should not be on the list at all
                for (const QString& group : groups.values)
                {
                    if (group != "Hidden")
                    {
                        EBusAggregateAssetTypesIfBelongsToGroup types(group);
                        AZ::AssetTypeInfoBus::BroadcastResult(types, &AZ::AssetTypeInfo::GetAssetType);

                        if (!types.values.empty())
                        {
                            AssetGroupFilter* groupFilter = new AssetGroupFilter();
                            groupFilter->SetAssetGroup(group);

                            AzQtComponents::SearchTypeFilter stFilter;
                            stFilter.displayName = group;
                            stFilter.metadata = QVariant::fromValue(FilterConstType(groupFilter));

                            filters.push_back(stFilter);
                        }
                    }
                }

                std::sort(filters.begin(), filters.end(),
                          [](const AzQtComponents::SearchTypeFilter& a, const AzQtComponents::SearchTypeFilter& b)
                {
                    const int categoryResult = QString::compare(a.category, b.category, Qt::CaseInsensitive);

                    if (categoryResult != 0)
                    {
                        return categoryResult < 0;
                    }
                    else if (a.displayName == QStringLiteral("Other"))
                    {
                        return false;
                    }
                    else if (b.displayName == QStringLiteral("Other"))
                    {
                        return true;
                    }

                    return QString::compare(a.displayName, b.displayName, Qt::CaseInsensitive) < 0;
                });

                return filters;
            }
        }

        SearchWidget::SearchWidget(QWidget* parent)
            : AzQtComponents::FilteredSearchWidget(parent)
            , m_filter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND))
            , m_stringFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND))
            , m_typesFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::OR))
            , m_engineFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND))
            , m_unusableProductsFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND))
            , m_folderFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND))
        {
            m_filter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);

            m_stringFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Up);
            m_stringFilter->SetTag("String");

            m_typesFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
            m_typesFilter->SetTag("AssetTypes");

            m_engineFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
            m_engineFilter->SetTag("ProjectAssets");

            m_unusableProductsFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
            m_unusableProductsFilter->SetTag("UnusableProducts");

            m_folderFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
            m_folderFilter->SetTag("Folder");

            connect(this, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this,
                    [this](const QString& text)
            {
                if (!filterLineEdit()->isHidden())
                {
                    m_stringFilter->RemoveAllFilters();

                    auto stringList = text.split(' ', Qt::SkipEmptyParts);
                    for (auto& str : stringList)
                    {
                        auto stringFilter = new StringFilter();
                        stringFilter->SetFilterString(str);
                        m_stringFilter->AddFilter(FilterConstType(stringFilter));
                    }
                }
            });
            connect(this, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this,
                    [this](const AzQtComponents::SearchTypeFilterList& filters)
            {
                if (!filterTypePushButton()->isHidden())
                {
                    m_typesFilter->RemoveAllFilters();

                    if (filters.isEmpty())
                    {
                        m_typesFilter->SetEmptyResult(true);
                    }
                    else
                    {
                        for (auto it = filters.constBegin(), end = filters.constEnd(); it != end; ++it)
                        {
                            m_typesFilter->AddFilter((*it).metadata.value<FilterConstType>());
                        }
                    }
                }
            });
        }

        void SearchWidget::Setup(bool stringFilter, bool assetTypeFilter, bool useFavorites)
        {
            ClearTextFilter();
            ClearTypeFilter();

            m_filter->RemoveAllFilters();

            SetTextFilterVisible(stringFilter);
            SetTypeFilterVisible(assetTypeFilter);
            SetUseFavorites(useFavorites);

            if (stringFilter)
            {
                m_filter->AddFilter(m_stringFilter);
            }

            // do not show assets in Hidden group
            auto hiddenGroupFilter = new AssetGroupFilter();
            hiddenGroupFilter->SetAssetGroup("Hidden");
            auto inverseFilter = new InverseFilter();
            inverseFilter->SetFilter(FilterConstType(hiddenGroupFilter));
            m_filter->AddFilter(FilterConstType(inverseFilter));

            // hide irrelevant
            auto cleanerProductsFilter = new CleanerProductsFilter();
            m_filter->AddFilter(FilterConstType(cleanerProductsFilter));

            if (assetTypeFilter)
            {
                m_filter->AddFilter(FilterConstType(m_typesFilter));
                SetTypeFilters(buildTypesFilterList());
            }

            const AZ::IO::Path enginePath{ AZ::Utils::GetEnginePath() };
            AZ::IO::Path engineAssets =  (enginePath / "Assets" / "Engine").LexicallyNormal();
            AZ::IO::Path setregAssets = (enginePath / "Registry").LexicallyNormal();

            auto filterFn = [engineAssets, setregAssets](const AssetBrowserEntry* entry)
            {
                AZ::IO::Path absolutePath = entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product
                    ? entry->GetParent()->GetFullPath()
                    : entry->GetFullPath();

                return !(absolutePath.IsRelativeTo(engineAssets) || absolutePath.IsRelativeTo(setregAssets));
            };
            auto engineFilter = new CustomFilter(filterFn);
            m_engineFilter->AddFilter(FilterConstType(engineFilter));

            AZStd::vector<AZ::Data::AssetType> types = BuildAssetTypeList();
            auto productFilterFn = [types](const AssetBrowserEntry* entry)
            {
                if (const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(entry))
                {
                    return !AZStd::any_of(
                        types.begin(),
                        types.end(),
                        [&](const auto& type)
                        {
                            return productEntry->GetAssetType() == type;
                        });
                }
                return true;
            };
            auto productFilter = new CustomFilter(productFilterFn);
            m_unusableProductsFilter->AddFilter(FilterConstType(productFilter));

            auto directoryFilter = new EntryTypeFilter();
            directoryFilter->SetName("Folder");
            directoryFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Folder);
            m_folderFilter->AddFilter(FilterConstType(directoryFilter));
        }

        void SearchWidget::ToggleEngineFilter(bool checked)
        {
            if (!checked)
            {
                if (GetIsEngineFilterActive())
                {
                    m_filter->RemoveFilter(FilterConstType(m_engineFilter));
                }
            }
            else
            {
                if (!GetIsEngineFilterActive())
                {
                    m_filter->AddFilter(FilterConstType(m_engineFilter));
                }
            }
        }

        void SearchWidget::ToggleUnusableProductsFilter(bool checked)
        {
            if (!checked)
            {
                if (GetIsUnusableProductsFilterActive())
                {
                    m_filter->RemoveFilter(FilterConstType(m_unusableProductsFilter));
                }
            }
            else
            {
                if (!GetIsUnusableProductsFilterActive())
                {
                    m_filter->AddFilter(FilterConstType(m_unusableProductsFilter));
                }
            }
        }

        AZStd::vector<AZ::Data::AssetType> SearchWidget::BuildAssetTypeList()
        {
            AZStd::vector<AZ::Data::AssetType> entries;
            if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                auto VisitFilteredAssetTypes = [&entries](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
                {
                    using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
                    if (FixedValueString value{}; visitArgs.m_registry.Get(value, visitArgs.m_jsonKeyPath))
                    {
                        AZ::Data::AssetType assetType{ AZStd::string_view(value) };
                        entries.emplace_back(AZStd::move(assetType));
                    }
                    return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                };
                AZ::SettingsRegistryVisitorUtils::VisitObject(
                    *settingsRegistry, VisitFilteredAssetTypes, "/O3DE/AssetBrowser/AssetTypeUuidExcludes");
            }

            return entries;
        }

        void SearchWidget::AddFolderFilter()
        {
            if (!m_filter->GetSubFilters().contains(m_folderFilter))
            {
                m_filter->AddFilter(FilterConstType(m_folderFilter));
            }
        }

        void SearchWidget::RemoveFolderFilter()
        {
            if (m_filter->GetSubFilters().contains(m_folderFilter))
            {
                m_filter->RemoveFilter(FilterConstType(m_folderFilter));
            }
        }

        QSharedPointer<CompositeFilter> SearchWidget::GetFilter() const
        {
            return m_filter;
        }

        QSharedPointer<CompositeFilter> SearchWidget::GetStringFilter() const
        {
            return m_stringFilter;
        }

        QSharedPointer<CompositeFilter> SearchWidget::GetTypesFilter() const
        {
            return m_typesFilter;
        }

        QSharedPointer<CompositeFilter> SearchWidget::GetEngineFilter() const
        {
            return m_engineFilter;
        }

        QSharedPointer<CompositeFilter> SearchWidget::GetUnusableProductsFilter() const
        {
            return m_unusableProductsFilter;
        }

        QSharedPointer<CompositeFilter> SearchWidget::GetFolderFilter() const
        {
            return m_folderFilter;
        }

        bool SearchWidget::GetIsEngineFilterActive()
        {
            return m_filter->GetSubFilters().contains(m_engineFilter);
        }

        bool SearchWidget::GetIsUnusableProductsFilterActive()
        {
            return m_filter->GetSubFilters().contains(m_unusableProductsFilter);
        }

        bool SearchWidget::GetIsFolderFilterActive()
        {
            return m_filter->GetSubFilters().contains(m_folderFilter);
        }

        void SearchWidget::SetFilterString(const QString& searchTerm)
        {
            SetTextFilter(searchTerm);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Search/moc_SearchWidget.cpp"
