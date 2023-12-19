/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        namespace
        {
            inline bool StringMatch(const QString& searched, const QString& text)
            {
                return text.contains(searched, Qt::CaseInsensitive);
            }

            //! Expand all entries that are either parent or child relationship to the entry and write to result
            void Expand(AZStd::unordered_set<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry)
            {
                entry->VisitUp(
                    [&](const auto& currentEntry)
                    {
                        result.insert(currentEntry);
                        return true;
                    });

                entry->VisitDown(
                    [&](const auto& currentEntry)
                    {
                        result.reserve(result.size() + currentEntry->GetChildCount() + 1);
                        result.insert(currentEntry);
                        return true;
                    });
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetBrowserEntryFilter
        //////////////////////////////////////////////////////////////////////////
        bool AssetBrowserEntryFilter::Match(const AssetBrowserEntry* entry) const
        {
            if (m_direction == None)
            {
                if (MatchInternal(entry))
                {
                    return true;
                }
            }

            if (m_direction & Up)
            {
                bool result = false;
                entry->VisitUp(
                    [&result, this](const auto& currentEntry)
                    {
                        if (MatchInternal(currentEntry))
                        {
                            result = true;
                            return false;
                        }
                        return true;
                    });

                if (result)
                {
                    return true;
                }
            }

            if (m_direction & Down)
            {
                bool result = false;
                entry->VisitDown(
                    [&](const auto& currentEntry)
                    {
                        if (MatchInternal(currentEntry))
                        {
                            result = true;
                            return false;
                        }
                        return true;
                    });

                if (result)
                {
                    return true;
                }
            }

            return false;
        }

        bool AssetBrowserEntryFilter::MatchWithoutPropagation(const AssetBrowserEntry* entry) const
        {
            return MatchInternal(entry);
        }

        void AssetBrowserEntryFilter::Filter(AZStd::unordered_set<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const
        {
            if (m_direction == None)
            {
                if (MatchInternal(entry))
                {
                    Expand(result, entry);
                    return;
                }
            }

            if (m_direction & Up)
            {
                entry->VisitUp(
                    [&result, this](const auto& currentEntry)
                    {
                        if (MatchInternal(currentEntry))
                        {
                            Expand(result, currentEntry);
                            return false;
                        }
                        return true;
                    });
            }

            if (m_direction & Down)
            {
                entry->VisitDown(
                    [&](const auto& currentEntry)
                    {
                        if (MatchInternal(currentEntry))
                        {
                            Expand(result, currentEntry);
                            return false;
                        }
                        return true;
                    });
            }
        }

        QString AssetBrowserEntryFilter::GetName() const
        {
            return m_name.isEmpty() ? GetNameInternal() : m_name;
        }

        void AssetBrowserEntryFilter::SetName(const QString& name)
        {
            m_name = name;
        }

        const QString& AssetBrowserEntryFilter::GetTag() const
        {
            return m_tag;
        }

        void AssetBrowserEntryFilter::SetTag(const QString& tag)
        {
            m_tag = tag;
        }

        void AssetBrowserEntryFilter::SetFilterPropagation(int direction)
        {
            m_direction = direction;
        }

        //////////////////////////////////////////////////////////////////////////
        // StringFilter
        //////////////////////////////////////////////////////////////////////////
        AssetBrowserEntryFilter* StringFilter::Clone() const
        {
            auto clone = new StringFilter();
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            clone->m_filterString = m_filterString;
            return clone;
        }

        void StringFilter::SetFilterString(const QString& filterString)
        {
            m_filterString = filterString;
            Q_EMIT updatedSignal();
        }

        QString StringFilter::GetFilterString() const
        {
            return m_filterString;
        }

        QString StringFilter::GetNameInternal() const
        {
            return m_filterString;
        }

        bool StringFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            // Return true if the filter is empty or the display name matches.
            return m_filterString.isEmpty() || StringMatch(m_filterString, entry->GetDisplayName());
        }

        //////////////////////////////////////////////////////////////////////////
        // CustomFilter
        //////////////////////////////////////////////////////////////////////////
        CustomFilter::CustomFilter(const AZStd::function<bool(const AssetBrowserEntry*)>& filterFn)
            : m_filterFn(filterFn)
        {
        }

        AssetBrowserEntryFilter* CustomFilter::Clone() const
        {
            auto clone = new CustomFilter(m_filterFn);
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            return clone;
        }

        QString CustomFilter::GetNameInternal() const
        {
            return "Custom Filter";
        }

        bool CustomFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            return !m_filterFn || m_filterFn(entry);
        }

        //////////////////////////////////////////////////////////////////////////
        // RegExpFilter
        //////////////////////////////////////////////////////////////////////////
        AssetBrowserEntryFilter* RegExpFilter::Clone() const
        {
            auto clone = new RegExpFilter();
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            clone->m_filterPattern = m_filterPattern;
            return clone;
        }

        void RegExpFilter::SetFilterPattern(const QRegExp& filterPattern)
        {
            m_filterPattern = filterPattern;
            Q_EMIT updatedSignal();
        }

        QString RegExpFilter::GetNameInternal() const
        {
            return m_filterPattern.pattern();
        }

        bool RegExpFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            // Return true if the filter is empty or the display name matches.
            return m_filterPattern.isEmpty() || m_filterPattern.exactMatch(entry->GetDisplayName());
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetTypeFilter
        //////////////////////////////////////////////////////////////////////////
        AssetBrowserEntryFilter* AssetTypeFilter::Clone() const
        {
            auto clone = new AssetTypeFilter();
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            clone->m_assetType = m_assetType;
            return clone;
        }

        void AssetTypeFilter::SetAssetType(AZ::Data::AssetType assetType)
        {
            m_assetType = assetType;
            Q_EMIT updatedSignal();
        }

        void AssetTypeFilter::SetAssetType(const char* assetTypeName)
        {
            EBusFindAssetTypeByName result(assetTypeName);
            AZ::AssetTypeInfoBus::BroadcastResult(result, &AZ::AssetTypeInfo::GetAssetType);
            SetAssetType(result.GetAssetType());
        }

        AZ::Data::AssetType AssetTypeFilter::GetAssetType() const
        {
            return m_assetType;
        }

        QString AssetTypeFilter::GetNameInternal() const
        {
            QString name;
            AZ::AssetTypeInfoBus::EventResult(name, m_assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
            return name;
        }

        bool AssetTypeFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            // this filter only works on products.
            const ProductAssetBrowserEntry* product = entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product
                ? static_cast<const ProductAssetBrowserEntry*>(entry)
                : nullptr;
            if (!product)
            {
                return false;
            }

            return m_assetType.IsNull() || (product && product->GetAssetType() == m_assetType);
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetGroupFilter
        //////////////////////////////////////////////////////////////////////////
        AssetBrowserEntryFilter* AssetGroupFilter::Clone() const
        {
            auto clone = new AssetGroupFilter();
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            clone->SetAssetGroup(m_group);
            return clone;
        }

        void AssetGroupFilter::SetAssetGroup(const QString& group)
        {
            m_group = group;
            m_groupCrc = AZ::Crc32(group.toUtf8().constData());
            m_groupIsAll = m_group.compare("All", Qt::CaseInsensitive) == 0;
            m_groupIsOther = m_group.compare("Other", Qt::CaseInsensitive) == 0;
        }

        const QString& AssetGroupFilter::GetAssetTypeGroup() const
        {
            return m_group;
        }

        QString AssetGroupFilter::GetNameInternal() const
        {
            return m_group;
        }

        bool AssetGroupFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            // this filter only works on products.
            const ProductAssetBrowserEntry* product = entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product
                ? static_cast<const ProductAssetBrowserEntry*>(entry)
                : nullptr;
            if (!product)
            {
                return false;
            }

            return (m_groupIsAll) || (m_groupIsOther && product->GetGroupName().isEmpty()) || (m_groupCrc == product->GetGroupNameCrc());
        }

        //////////////////////////////////////////////////////////////////////////
        // EntryTypeFilter
        //////////////////////////////////////////////////////////////////////////
        EntryTypeFilter::EntryTypeFilter()
            : m_entryType(AssetBrowserEntry::AssetEntryType::Product)
        {
        }

        AssetBrowserEntryFilter* EntryTypeFilter::Clone() const
        {
            auto clone = new EntryTypeFilter();
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            clone->m_entryType = m_entryType;
            return clone;
        }

        void EntryTypeFilter::SetEntryType(AssetBrowserEntry::AssetEntryType entryType)
        {
            m_entryType = entryType;
        }

        AssetBrowserEntry::AssetEntryType EntryTypeFilter::GetEntryType() const
        {
            return m_entryType;
        }

        QString EntryTypeFilter::GetNameInternal() const
        {
            return AssetBrowserEntry::AssetEntryTypeToString(m_entryType);
        }

        bool EntryTypeFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            return entry->GetEntryType() == m_entryType;
        }

        //////////////////////////////////////////////////////////////////////////
        // CompositeFilter
        //////////////////////////////////////////////////////////////////////////
        CompositeFilter::CompositeFilter(LogicOperatorType logicOperator)
            : m_logicOperator(logicOperator)
        {
        }

        AssetBrowserEntryFilter* CompositeFilter::Clone() const
        {
            auto clone = new CompositeFilter();
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            clone->m_logicOperator = m_logicOperator;
            clone->m_emptyResult = m_emptyResult;
            for (const auto& subFilter : m_subFilters)
            {
                clone->AddFilter(FilterConstType(subFilter->Clone()));
            }
            return clone;
        }

        void CompositeFilter::AddFilter(FilterConstType filter)
        {
            connect(filter.data(), &AssetBrowserEntryFilter::updatedSignal, this, &AssetBrowserEntryFilter::updatedSignal, Qt::UniqueConnection);
            m_subFilters.append(filter);
            Q_EMIT updatedSignal();
        }

        void CompositeFilter::RemoveFilter(FilterConstType filter)
        {
            if (m_subFilters.removeAll(filter))
            {
                Q_EMIT updatedSignal();
            }
        }

        void CompositeFilter::RemoveAllFilters()
        {
            m_subFilters.clear();
            Q_EMIT updatedSignal();
        }

        void CompositeFilter::SetLogicOperator(LogicOperatorType logicOperator)
        {
            m_logicOperator = logicOperator;
            Q_EMIT updatedSignal();
        }

        const QList<FilterConstType>& CompositeFilter::GetSubFilters() const
        {
            return m_subFilters;
        }

        void CompositeFilter::SetEmptyResult(bool result)
        {
            if (m_emptyResult != result)
            {
                m_emptyResult = result;
                Q_EMIT updatedSignal();
            }
        }

        QString CompositeFilter::GetNameInternal() const
        {
            QString name;
            for (auto it = m_subFilters.begin(); it != m_subFilters.end(); ++it)
            {
                name += (*it)->GetName();
                if (AZStd::next(it) != m_subFilters.end())
                {
                    name += ", ";
                }
            }
            return name;
        }

        bool CompositeFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            if (m_subFilters.empty())
            {
                return m_emptyResult;
            }

            // AND
            if (m_logicOperator == LogicOperatorType::AND)
            {
                return AZStd::all_of(m_subFilters.begin(), m_subFilters.end(), [&entry](const auto& filter) {
                    return filter->Match(entry);
                });
            }
            // OR
            return AZStd::any_of(m_subFilters.begin(), m_subFilters.end(), [&entry](const auto& filter) {
                return filter->Match(entry);
            });
        }

        //////////////////////////////////////////////////////////////////////////
        // InverseFilter
        //////////////////////////////////////////////////////////////////////////
        AssetBrowserEntryFilter* InverseFilter::Clone() const
        {
            auto clone = new InverseFilter();
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            clone->m_filter = FilterConstType(m_filter->Clone());
            return clone;
        }

        void InverseFilter::SetFilter(FilterConstType filter)
        {
            if (m_filter != filter)
            {
                m_filter = filter;
                Q_EMIT updatedSignal();
            }
        }

        QString InverseFilter::GetNameInternal() const
        {
            return m_filter.isNull() ? tr("NOT") : tr("NOT (%1)").arg(m_filter->GetName());
        }

        bool InverseFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            return m_filter && !m_filter->Match(entry);
        }

        //////////////////////////////////////////////////////////////////////////
        // CleanerProductsFilter
        //////////////////////////////////////////////////////////////////////////
        AssetBrowserEntryFilter* CleanerProductsFilter::Clone() const
        {
            auto clone = new CleanerProductsFilter();
            clone->m_name = m_name;
            clone->m_tag = m_tag;
            clone->m_direction = m_direction;
            return clone;
        }

        QString CleanerProductsFilter::GetNameInternal() const
        {
            return QString();
        }

        bool CleanerProductsFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            const ProductAssetBrowserEntry* product = entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product
                ? static_cast<const ProductAssetBrowserEntry*>(entry)
                : nullptr;
            if (!product)
            {
                return true;
            }

            auto source = product->GetParent();
            if (!source || source->GetChildCount() != 1)
            {
                return true;
            }

            bool result = false;
            AZ::AssetTypeInfoBus::Event(
                product->GetAssetType(),
                [&result](AZ::AssetTypeInfoBus::Events* handler)
                {
                    const char* assetTypeName = handler->GetAssetTypeDisplayName();
                    result = assetTypeName && assetTypeName[0];
                });
            return result;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Search/moc_Filter.cpp"
