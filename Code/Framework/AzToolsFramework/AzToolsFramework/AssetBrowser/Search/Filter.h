/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

#include <QObject>
#include <QSharedPointer>
#include <QString>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntryFilter;
        typedef QSharedPointer<const AssetBrowserEntryFilter> FilterConstType;

        //////////////////////////////////////////////////////////////////////////
        // AssetBrowserEntryFilter
        //////////////////////////////////////////////////////////////////////////
        //! Filters are used to fascilitate searching asset browser for specific asset
        //! They are also used for enforcing selection constraints for asset picking
        class AssetBrowserEntryFilter : public QObject
        {
            Q_OBJECT
        public:
            //! Propagate direction allows match satisfaction based on entry parents and/or children
            /*
                if PropagateDirection = Down, and entry does not satisfy filter, evaluation will propagate recursively to its children
                until at least one child satisfies the filter, then the original entry would match
                if PropagateDirection = Up, and entry does not satisfy filter, evaluation will propagate recursively upwards to its parents
                until first parent matches the filter, then the original entry would match
                if PropagateDirection = None, only entry itself is considered by the filter
            */
            enum class PropagateDirection : uint32_t
            {
                None,
                Up,
                Down,
                Both
            };

            AssetBrowserEntryFilter() = default;
            ~AssetBrowserEntryFilter() override = default;

            //! Cloning function that must be overridden for certain asset browser views that duplicate and modify incoming filters
            //! This should be implemented using a copy constructor, which is currently not possible because inheriting QObject prevents it.
            virtual AssetBrowserEntryFilter* Clone() const = 0;

            //! Check if entry matches filter
            bool Match(const AssetBrowserEntry* entry) const;

            //! Check if the entry matches filter without propagation (i.e. it's an exact match and it doesn't match only
            //  beause a descendant or an ancestor matches)
            bool MatchWithoutPropagation(const AssetBrowserEntry* entry) const;

            //! Retrieve all matching entries that are either entry itself or its parents or children
            void Filter(AZStd::unordered_set<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const;

            //! Filter name is used to uniquely identify the filter
            QString GetName() const;
            void SetName(const QString& name);

            //! Tags are used for identifying filter groups
            const QString& GetTag() const;
            void SetTag(const QString& tag);

            void SetFilterPropagation(PropagateDirection direction);

        Q_SIGNALS:
            //! Emitted every time a filter is updated, in case of composite filter, the signal is propagated to the top level filter so
            //! only one listener needs to connected
            void updatedSignal() const;

        protected:
            //! Internal name auto generated based on filter type and data
            virtual QString GetNameInternal() const = 0;

            //! Internal matching logic overrided by every filter type
            virtual bool MatchInternal(const AssetBrowserEntry* entry) const;

            //! Internal filtering logic overrided by every filter type
            virtual void FilterInternal(AZStd::unordered_set<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const;

        protected:
            QString m_name;
            QString m_tag;
            PropagateDirection m_direction{ PropagateDirection::None };
        };

        //////////////////////////////////////////////////////////////////////////
        // StringFilter
        //////////////////////////////////////////////////////////////////////////
        //! StringFilter filters assets based on their name
        class StringFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            StringFilter() = default;
            ~StringFilter() override = default;
            AssetBrowserEntryFilter* Clone() const override;

            void SetFilterString(const QString& filterString);
            QString GetFilterString() const;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            QString m_filterString;
        };

        //////////////////////////////////////////////////////////////////////////
        // CustomFilter
        //////////////////////////////////////////////////////////////////////////
        //! CustomFilter filters assets based on a custom filter function
        class CustomFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            CustomFilter(const AZStd::function<bool(const AssetBrowserEntry*)>& filterFn);
            ~CustomFilter() override = default;
            AssetBrowserEntryFilter* Clone() const override;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            AZStd::function<bool(const AssetBrowserEntry*)> m_filterFn;
        };

        //////////////////////////////////////////////////////////////////////////
        // CustomFilter
        //////////////////////////////////////////////////////////////////////////
        //! RegExpFilter filters assets based on a regular expression pattern
        class RegExpFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            RegExpFilter() = default;
            ~RegExpFilter() override = default;
            AssetBrowserEntryFilter* Clone() const override;

            void SetFilterPattern(const QRegExp& filterPattern);

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            QRegExp m_filterPattern;
        };

        //////////////////////////////////////////////////////////////////////////
        // AssetTypeFilter
        //////////////////////////////////////////////////////////////////////////
        //! AssetTypeFilter filters products based on their asset type
        class AssetTypeFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            AssetTypeFilter() = default;
            ~AssetTypeFilter() override = default;
            AssetBrowserEntryFilter* Clone() const override;

            void SetAssetType(AZ::Data::AssetType assetType);
            void SetAssetType(const char* assetTypeName);
            AZ::Data::AssetType GetAssetType() const;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            AZ::Data::AssetType m_assetType{ AZ::Data::AssetType::CreateNull() };
        };

        //////////////////////////////////////////////////////////////////////////
        // AssetGroupFilter
        //////////////////////////////////////////////////////////////////////////
        //! AssetGroupFilter filters products based on their asset group
        class AssetGroupFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            AssetGroupFilter() = default;
            ~AssetGroupFilter() override = default;
            AssetBrowserEntryFilter* Clone() const override;

            void SetAssetGroup(const QString& group);
            const QString& GetAssetTypeGroup() const;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            QString m_group{ "All" };
            AZ::u32 m_groupCrc{ AZ::Crc32("All") };
            bool m_groupIsAll{ true };
            bool m_groupIsOther{ false };
        };

        //////////////////////////////////////////////////////////////////////////
        // EntryTypeFilter
        //////////////////////////////////////////////////////////////////////////
        class EntryTypeFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            EntryTypeFilter();
            ~EntryTypeFilter() override = default;
            AssetBrowserEntryFilter* Clone() const override;

            void SetEntryType(AssetBrowserEntry::AssetEntryType entryType);
            AssetBrowserEntry::AssetEntryType GetEntryType() const;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            AssetBrowserEntry::AssetEntryType m_entryType;
        };

        //////////////////////////////////////////////////////////////////////////
        // CompositeFilter
        //////////////////////////////////////////////////////////////////////////
        //! CompositeFilter performs an AND/OR operation between multiple subfilters
        /*
            If more complex logic operations required, CompositeFilters can be nested
            with different logic operator types
        */
        class CompositeFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            enum class LogicOperatorType : uint32_t
            {
                OR,
                AND
            };

            explicit CompositeFilter(LogicOperatorType logicOperator);
            CompositeFilter() = default;
            ~CompositeFilter() override = default;
            AssetBrowserEntryFilter* Clone() const override;

            void AddFilter(FilterConstType filter);
            void RemoveFilter(FilterConstType filter);
            void RemoveAllFilters();
            void SetLogicOperator(LogicOperatorType logicOperator);
            const QList<FilterConstType>& GetSubFilters() const;
            //! Return value if there are no subfilters present
            void SetEmptyResult(bool result);

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            QList<FilterConstType> m_subFilters;
            LogicOperatorType m_logicOperator{ LogicOperatorType::AND };
            bool m_emptyResult{ true };
        };

        //////////////////////////////////////////////////////////////////////////
        // InverseFilter
        //////////////////////////////////////////////////////////////////////////
        //! Inverse filter negates result of its child filter
        class InverseFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            InverseFilter() = default;
            ~InverseFilter() override = default;

            AssetBrowserEntryFilter* Clone() const override;

            void SetFilter(FilterConstType filter);

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            FilterConstType m_filter;
        };

        //////////////////////////////////////////////////////////////////////////
        // CleanerProductsFilter
        //////////////////////////////////////////////////////////////////////////
        //! Filters out products that shouldn't be shown
        class CleanerProductsFilter : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            CleanerProductsFilter() = default;
            ~CleanerProductsFilter() override = default;

            AssetBrowserEntryFilter* Clone() const override;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;
        };

        template<class T>
        struct EBusAggregateUniqueResults
        {
            AZStd::vector<T> values;
            void operator=(const T& rhs)
            {
                if (AZStd::find(values.begin(), values.end(), rhs) == values.end())
                {
                    values.push_back(rhs);
                }
            }
        };

        struct EBusAggregateAssetTypesIfBelongsToGroup
        {
            EBusAggregateAssetTypesIfBelongsToGroup(const QString& group)
                : m_group(group)
            {
            }

            EBusAggregateAssetTypesIfBelongsToGroup(const EBusAggregateAssetTypesIfBelongsToGroup&) = delete;
            EBusAggregateAssetTypesIfBelongsToGroup& operator=(const EBusAggregateAssetTypesIfBelongsToGroup&) = delete;

            AZStd::vector<AZ::Data::AssetType> values;

            void operator=(const AZ::Data::AssetType& assetType)
            {
                if (BelongsToGroup(assetType))
                {
                    values.push_back(assetType);
                }
            }

        private:
            const QString& m_group;

            bool BelongsToGroup(const AZ::Data::AssetType& assetType)
            {
                QString group;
                AZ::AssetTypeInfoBus::EventResult(group, assetType, &AZ::AssetTypeInfo::GetGroup);
                return !group.compare(m_group, Qt::CaseInsensitive);
            }
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

Q_DECLARE_METATYPE(AzToolsFramework::AssetBrowser::FilterConstType)
