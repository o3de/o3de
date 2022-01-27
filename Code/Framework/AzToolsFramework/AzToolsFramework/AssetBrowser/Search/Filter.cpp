/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        namespace
        {
            bool StringMatch(const QString& searched, const QString& text)
            {
                return text.contains(searched, Qt::CaseInsensitive);
            }

            //! Intersect operation between two sets which then overwrites result
            void Intersect(AZStd::vector<const AssetBrowserEntry*>& result, AZStd::vector<const AssetBrowserEntry*>& set)
            {
                // inefficient, but sets are tiny so probably not worth the optimization effort
                AZStd::vector<const AssetBrowserEntry*> intersection;
                for (auto entry : result)
                {
                    if (AZStd::find(set.begin(), set.end(), entry) != set.end())
                    {
                        intersection.push_back(entry);
                    }
                }
                result = intersection;
            }

            //! Insert an entry if it doesn't already exist
            void Join(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry)
            {
                if (AZStd::find(result.begin(), result.end(), entry) == result.end())
                {
                    result.push_back(entry);
                }
            }

            //! Join operation between two sets which then overwrites result
            void Join(AZStd::vector<const AssetBrowserEntry*>& result, AZStd::vector<const AssetBrowserEntry*>& set)
            {
                AZStd::vector<const AssetBrowserEntry*> unionResult;
                for (auto entry : set)
                {
                    Join(result, entry);
                }
            }

            //! Expand all children recursively and write to result
            void ExpandDown(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry)
            {
                Join(result, entry);
                AZStd::vector<const AssetBrowserEntry*> children;
                entry->GetChildren<AssetBrowserEntry>(children);
                for (auto child : children)
                {
                    ExpandDown(result, child);
                }
            }

            //! Expand all entries that are either parent or child relationship to the entry and write to result
            void Expand(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry)
            {
                auto parent = entry->GetParent();
                while (parent && parent->GetEntryType() != AssetBrowserEntry::AssetEntryType::Root)
                {
                    Join(result, parent);
                    parent = parent->GetParent();
                }
                ExpandDown(result, entry);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetBrowserEntryFilter
        //////////////////////////////////////////////////////////////////////////
        AssetBrowserEntryFilter::AssetBrowserEntryFilter()
            : m_direction(None)
        {
        }

        bool AssetBrowserEntryFilter::Match(const AssetBrowserEntry* entry) const
        {
            if (MatchInternal(entry))
            {
                return true;
            }

            if (m_direction & Up)
            {
                auto parent = entry->GetParent();
                while (parent && parent->GetEntryType() != AssetBrowserEntry::AssetEntryType::Root)
                {
                    if (MatchInternal(parent))
                    {
                        return true;
                    }
                    parent = parent->GetParent();
                }
            }
            if (m_direction & Down)
            {
                AZStd::vector<const AssetBrowserEntry*> children;
                entry->GetChildren<AssetBrowserEntry>(children);
                for (auto child : children)
                {
                    if (MatchDown(child))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        void AssetBrowserEntryFilter::Filter(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const
        {
            FilterInternal(result, entry);

            if (m_direction & Up)
            {
                auto parent = entry->GetParent();
                while (parent && parent->GetEntryType() != AssetBrowserEntry::AssetEntryType::Root)
                {
                    FilterInternal(result, parent);
                    parent = parent->GetParent();
                }
            }
            if (m_direction & Down)
            {
                AZStd::vector<const AssetBrowserEntry*> children;
                entry->GetChildren<AssetBrowserEntry>(children);
                for (auto child : children)
                {
                    FilterDown(result, child);
                }
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

        void AssetBrowserEntryFilter::FilterInternal(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const
        {
            if (MatchInternal(entry))
            {
                Join(result, entry);
            }
        }

        bool AssetBrowserEntryFilter::MatchDown(const AssetBrowserEntry* entry) const
        {
            if (MatchInternal(entry))
            {
                return true;
            }
            AZStd::vector<const AssetBrowserEntry*> children;
            entry->GetChildren<AssetBrowserEntry>(children);
            for (auto child : children)
            {
                if (MatchDown(child))
                {
                    return true;
                }
            }
            return false;
        }

        void AssetBrowserEntryFilter::FilterDown(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const
        {
            if (MatchInternal(entry))
            {
                Join(result, entry);
            }
            AZStd::vector<const AssetBrowserEntry*> children;
            entry->GetChildren<AssetBrowserEntry>(children);
            for (auto child : children)
            {
                FilterDown(result, child);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // StringFilter
        //////////////////////////////////////////////////////////////////////////
        StringFilter::StringFilter()
            : m_filterString("") {}

        void StringFilter::SetFilterString(const QString& filterString)
        {
            m_filterString = filterString;
            Q_EMIT updatedSignal();
        }

        QString StringFilter::GetNameInternal() const
        {
            return m_filterString;
        }

        bool StringFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            // no filter string matches any asset
            if (m_filterString.isEmpty())
            {
                return true;
            }

            // entry's name matches search pattern
            if (StringMatch(m_filterString, entry->GetDisplayName()))
            {
                return true;
            }

            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        // RegExpFilter
        //////////////////////////////////////////////////////////////////////////
        RegExpFilter::RegExpFilter()
            : m_filterPattern("")
        {
        }

        void RegExpFilter::SetFilterPattern(const QString& filterPattern)
        {
            m_filterPattern = filterPattern;
            Q_EMIT updatedSignal();
        }

        QString RegExpFilter::GetNameInternal() const
        {
            return m_filterPattern;
        }

        bool RegExpFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            // no filter pattern matches any asset
            if (m_filterPattern.isEmpty())
            {
                return true;
            }

            // entry's name matches regular expression pattern
            QRegExp regExp(m_filterPattern);
            regExp.setPatternSyntax(QRegExp::Wildcard);

            return regExp.exactMatch(entry->GetDisplayName());
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetTypeFilter
        //////////////////////////////////////////////////////////////////////////
        AssetTypeFilter::AssetTypeFilter()
            : m_assetType(AZ::Data::AssetType::CreateNull()) {}

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
            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
            {
                if (m_assetType.IsNull())
                {
                    return true;
                }

                if (static_cast<const ProductAssetBrowserEntry*>(entry)->GetAssetType() == m_assetType)
                {
                    return true;
                }
            }
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetGroupFilter
        //////////////////////////////////////////////////////////////////////////
        AssetGroupFilter::AssetGroupFilter()
            : m_group("All")
        {
        }

        void AssetGroupFilter::SetAssetGroup(const QString& group)
        {
            m_group = group;
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
            if (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Product)
            {
                return false;
            }

            if (m_group.compare("All", Qt::CaseInsensitive) == 0)
            {
                return true;
            }

            auto product = static_cast<const ProductAssetBrowserEntry*>(entry);

            QString group;
            AZ::AssetTypeInfoBus::EventResult(group, product->GetAssetType(), &AZ::AssetTypeInfo::GetGroup);

            if (m_group.compare("Other", Qt::CaseInsensitive) == 0 && group.isEmpty())
            {
                return true;
            }

            return (m_group.compare(group, Qt::CaseInsensitive) == 0);
        }

        //////////////////////////////////////////////////////////////////////////
        // EntryTypeFilter
        //////////////////////////////////////////////////////////////////////////
        EntryTypeFilter::EntryTypeFilter()
            : m_entryType(AssetBrowserEntry::AssetEntryType::Product) {}

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
            , m_emptyResult(true) {}

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
            QString name = "";
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
            if (m_subFilters.count() == 0)
            {
                return m_emptyResult;
            }

            // AND
            if (m_logicOperator == LogicOperatorType::AND)
            {
                for (auto filter : m_subFilters)
                {
                    if (!filter->Match(entry))
                    {
                        return false;
                    }
                }
                return true;
            }
            // OR
            for (auto filter : m_subFilters)
            {
                if (filter->Match(entry))
                {
                    return true;
                }
            }
            return false;
        }

        void CompositeFilter::FilterInternal(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const
        {
            // if no subfilters are present in this composite filter then all relating entries would match
            if (m_subFilters.isEmpty())
            {
                // only if match on empty filter is success
                if (m_emptyResult)
                {
                    Expand(result, entry);
                }
                return;
            }

            // AND
            if (m_logicOperator == LogicOperatorType::AND)
            {
                AZStd::vector<const AssetBrowserEntry*> andResult;
                bool firstResult = true;

                for (auto filter : m_subFilters)
                {
                    if (firstResult)
                    {
                        firstResult = false;
                        filter->Filter(andResult, entry);
                    }
                    else
                    {
                        AZStd::vector<const AssetBrowserEntry*> set;
                        filter->Filter(set, entry);
                        Intersect(andResult, set);
                    }
                    if (andResult.empty())
                    {
                        break;
                    }
                }
                Join(result, andResult);
            }
            // OR
            else
            {
                for (auto filter : m_subFilters)
                {
                    AZStd::vector<const AssetBrowserEntry*> set;
                    filter->Filter(set, entry);
                    Join(result, set);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // InverseFilter
        //////////////////////////////////////////////////////////////////////////
        InverseFilter::InverseFilter() {}

        void InverseFilter::SetFilter(FilterConstType filter)
        {
            if (m_filter == filter)
            {
                return;
            }

            m_filter = filter;
            Q_EMIT updatedSignal();
        }

        QString InverseFilter::GetNameInternal() const
        {
            if (m_filter.isNull())
            {
                QString name = tr("NOT");
            }
            QString name = tr("NOT (%1)").arg(m_filter->GetName());
            return name;
        }

        bool InverseFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            if (m_filter.isNull())
            {
                return false;
            }
            return !m_filter->Match(entry);
        }

        void InverseFilter::FilterInternal(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const
        {
            if (MatchInternal(entry))
            {
                Expand(result, entry);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // CleanerProductsFilter
        //////////////////////////////////////////////////////////////////////////
        CleanerProductsFilter::CleanerProductsFilter() {}

        QString CleanerProductsFilter::GetNameInternal() const
        {
            return QString();
        }

        bool CleanerProductsFilter::MatchInternal(const AssetBrowserEntry* entry) const
        {
            auto product = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
            if (!product)
            {
                return true;
            }
            auto source = product->GetParent();
            if (!source)
            {
                return true;
            }
            if (source->GetChildCount() != 1)
            {
                return true;
            }

            AZStd::string assetTypeName;
            AZ::AssetTypeInfoBus::EventResult(assetTypeName, product->GetAssetType(), &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
            if (!assetTypeName.empty())
            {
                return true;
            }
            
            return false;
        }

        void CleanerProductsFilter::FilterInternal(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const
        {
            if (MatchInternal(entry))
            {
                Expand(result, entry);
            }
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Search/moc_Filter.cpp"
