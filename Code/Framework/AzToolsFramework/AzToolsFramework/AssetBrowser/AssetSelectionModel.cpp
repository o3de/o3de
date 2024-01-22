/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

#if !defined(Q_MOC_RUN)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QRegExp>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        namespace
        {
            QSharedPointer<AssetBrowserEntryFilter> CreateCompositeFilter_And(
                const AZStd::vector<QSharedPointer<AssetBrowserEntryFilter>>& filters,
                AssetBrowserEntryFilter::PropagateDirection direction = AssetBrowserEntryFilter::PropagateDirection::None)
            {
                QSharedPointer<CompositeFilter> compositeFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND));
                for (const auto& filter : filters)
                {
                    compositeFilter->AddFilter(filter);
                }
                compositeFilter->SetFilterPropagation(direction);
                return compositeFilter;
            }

            QSharedPointer<AssetBrowserEntryFilter> CreateCompositeFilter_Or(
                const AZStd::vector<QSharedPointer<AssetBrowserEntryFilter>>& filters,
                AssetBrowserEntryFilter::PropagateDirection direction = AssetBrowserEntryFilter::PropagateDirection::None)
            {
                QSharedPointer<CompositeFilter> compositeFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::OR));
                for (const auto& filter : filters)
                {
                    compositeFilter->AddFilter(filter);
                }
                compositeFilter->SetFilterPropagation(direction);
                return compositeFilter;
            }

            QSharedPointer<AssetBrowserEntryFilter> CreateEntryTypeFilter(
                const AZStd::vector<AssetBrowserEntry::AssetEntryType>& entryTypes,
                AssetBrowserEntryFilter::PropagateDirection direction = AssetBrowserEntryFilter::PropagateDirection::None)
            {
                QSharedPointer<CustomFilter> entryTypeFilter(new CustomFilter(
                    [entryTypes](const AssetBrowserEntry* entry)
                    {
                        return entryTypes.empty() ||
                            (entry && AZStd::find(entryTypes.begin(), entryTypes.end(), entry->GetEntryType()) != entryTypes.end());
                    }));
                entryTypeFilter->SetFilterPropagation(direction);
                return entryTypeFilter;
            }

            QSharedPointer<AssetBrowserEntryFilter> CreateAssetTypeFilter(
                const AZStd::vector<AZ::Data::AssetType>& assetTypes,
                AssetBrowserEntryFilter::PropagateDirection direction = AssetBrowserEntryFilter::PropagateDirection::None)
            {
                QSharedPointer<CustomFilter> assetTypeFilter(new CustomFilter(
                    [assetTypes](const AssetBrowserEntry* entry)
                    {
                        const ProductAssetBrowserEntry* product = entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product
                            ? static_cast<const ProductAssetBrowserEntry*>(entry)
                            : nullptr;
                        return assetTypes.empty() ||
                            (product && AZStd::find(assetTypes.begin(), assetTypes.end(), product->GetAssetType()) != assetTypes.end());
                    }));
                assetTypeFilter->SetFilterPropagation(direction);
                return assetTypeFilter;
            }
        } // namespace

        AssetSelectionModel::AssetSelectionModel()
            : m_multiselect(false)
        {
        }

        FilterConstType AssetSelectionModel::GetSelectionFilter() const
        {
            return m_selectionFilter;
        }

        void AssetSelectionModel::SetSelectionFilter(FilterConstType filter)
        {
            m_selectionFilter = filter;
        }

        FilterConstType AssetSelectionModel::GetDisplayFilter() const
        {
            return m_displayFilter;
        }

        void AssetSelectionModel::SetDisplayFilter(FilterConstType filter)
        {
            m_displayFilter = filter;
        }

        bool AssetSelectionModel::GetMultiselect() const
        {
            return m_multiselect;
        }

        void AssetSelectionModel::SetMultiselect(bool multiselect)
        {
            m_multiselect = multiselect;
        }

        const AZStd::vector<AZ::Data::AssetId>& AssetSelectionModel::GetSelectedAssetIds() const
        {
            return m_selectedAssetIds;
        }

        void AssetSelectionModel::SetSelectedAssetIds(const AZStd::vector<AZ::Data::AssetId>& selectedAssetIds)
        {
            m_selectedFilePaths.clear();
            m_selectedAssetIds = selectedAssetIds;
        }

        void AssetSelectionModel::SetSelectedAssetId(const AZ::Data::AssetId& selectedAssetId)
        {
            m_selectedFilePaths.clear();
            m_selectedAssetIds.clear();
            m_selectedAssetIds.push_back(selectedAssetId);
        }

        const AZStd::vector<AZStd::string>& AssetSelectionModel::GetSelectedFilePaths() const
        {
            return m_selectedFilePaths;
        }

        void AssetSelectionModel::SetSelectedFilePaths(const AZStd::vector<AZStd::string>& selectedFilePaths)
        {
            m_selectedAssetIds.clear();
            m_selectedFilePaths = selectedFilePaths;
        }

        void AssetSelectionModel::SetSelectedFilePath(const AZStd::string& selectedFilePath)
        {
            m_selectedAssetIds.clear();
            m_selectedFilePaths.clear();
            m_selectedFilePaths.push_back(selectedFilePath);
        }

        void AssetSelectionModel::SetDefaultDirectory(AZStd::string_view defaultDirectory)
        {
            m_defaultDirectory = defaultDirectory;
        }

        AZStd::string_view AssetSelectionModel::GetDefaultDirectory() const
        {
            return m_defaultDirectory;
        }

        AZStd::vector<const AssetBrowserEntry*>& AssetSelectionModel::GetResults()
        {
            return m_results;
        }

        const AssetBrowserEntry* AssetSelectionModel::GetResult()
        {
            return !m_results.empty() ? m_results.front() : nullptr;
        }

        bool AssetSelectionModel::IsValid() const
        {
            return !m_results.empty();
        }

        void AssetSelectionModel::SetTitle(const QString& title)
        {
            m_title = title;
        }

        QString AssetSelectionModel::GetTitle() const
        {
            if (!m_title.isEmpty())
            {
                return m_title;
            }

            auto displayFilter = GetDisplayFilter();
            if (displayFilter && !displayFilter->GetName().isEmpty())
            {
                return displayFilter->GetName();
            }

            return "Asset";
        }

        AssetSelectionModel AssetSelectionModel::AssetTypeSelection(
            const AZ::Data::AssetType& assetType, bool multiselect, bool supportSelectingSources)
        {
            return AssetTypeSelection(AZStd::vector<AZ::Data::AssetType>{ assetType }, multiselect, supportSelectingSources);
        }

        AssetSelectionModel AssetSelectionModel::AssetTypeSelection(
            const char* assetTypeName, bool multiselect, bool supportSelectingSources)
        {
            EBusFindAssetTypeByName result(assetTypeName);
            AZ::AssetTypeInfoBus::BroadcastResult(result, &AZ::AssetTypeInfo::GetAssetType);
            return AssetTypeSelection(result.GetAssetType(), multiselect, supportSelectingSources);
        }

        AssetSelectionModel AssetSelectionModel::AssetTypeSelection(
            const AZStd::vector<AZ::Data::AssetType>& assetTypes, bool multiselect, bool supportSelectingSources)
        {
            auto entryTypeFilter = supportSelectingSources
                ? CreateEntryTypeFilter({ AssetBrowserEntry::AssetEntryType::Product, AssetBrowserEntry::AssetEntryType::Source })
                : CreateEntryTypeFilter({ AssetBrowserEntry::AssetEntryType::Product });

            auto assetTypeFilter = supportSelectingSources
                ? CreateAssetTypeFilter(assetTypes, AssetBrowserEntryFilter::PropagateDirection::Down)
                : CreateAssetTypeFilter(assetTypes, AssetBrowserEntryFilter::PropagateDirection::None);

            auto selectionFilter = CreateCompositeFilter_And({ assetTypeFilter, entryTypeFilter });

            AssetSelectionModel selection;
            selection.SetDisplayFilter(assetTypeFilter);
            selection.SetSelectionFilter(selectionFilter);
            selection.SetMultiselect(multiselect);
            return selection;
        }

        AssetSelectionModel AssetSelectionModel::SourceAssetTypeSelection(const QRegExp& pattern, bool multiselect)
        {
            QSharedPointer<RegExpFilter> patternFilter(new RegExpFilter());
            patternFilter->SetFilterPattern(pattern);

            QSharedPointer<AssetBrowserEntryFilter> compositeFilter(
                CreateCompositeFilter_And({ CreateEntryTypeFilter({ AssetBrowserEntry::AssetEntryType::Source }), patternFilter }));

            AssetSelectionModel selection;
            selection.SetDisplayFilter(compositeFilter);
            selection.SetSelectionFilter(compositeFilter);
            selection.SetMultiselect(multiselect);
            return selection;
        }

        AssetSelectionModel AssetSelectionModel::EverythingSelection(bool multiselect)
        {
            AssetSelectionModel selection;
            selection.SetDisplayFilter(CreateCompositeFilter_Or({}));
            selection.SetSelectionFilter(CreateEntryTypeFilter({}));
            selection.SetMultiselect(multiselect);
            return selection;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework.
