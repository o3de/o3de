/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <QString>

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/algorithm.h>
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
        class AssetBrowserEntryFilter
            : public QObject
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
            enum PropagateDirection : int
            {
                None    = 0x00,
                Up      = 0x01,
                Down    = 0x02
            };

            AssetBrowserEntryFilter();
            virtual ~AssetBrowserEntryFilter() =  default;

            //! Check if entry matches filter
            bool Match(const AssetBrowserEntry* entry) const;

            //! Retrieve all matching entries that are either entry itself or its parents or children
            void Filter(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const;

            //! Filter name is used to uniquely identify the filter
            QString GetName() const;
            void SetName(const QString& name);

            //! Tags are used for identifying filter groups
            const QString& GetTag() const;
            void SetTag(const QString& tag);

            void SetFilterPropagation(int direction);

        Q_SIGNALS:
            //! Emitted every time a filter is updated, in case of composite filter, the signal is propagated to the top level filter so only one listener needs to connected
            void updatedSignal() const;

        protected:
            //! Internal name auto generated based on filter type and data
            virtual QString GetNameInternal() const = 0;
            //! Internal matching logic overrided by every filter type
            virtual bool MatchInternal(const AssetBrowserEntry* entry) const = 0;
            //! Internal filtering logic overrided by every filter type
            virtual void FilterInternal(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const;

        private:
            QString m_name;
            QString m_tag;
            int m_direction;

            bool MatchDown(const AssetBrowserEntry* entry) const;
            void FilterDown(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const;
        };


        //////////////////////////////////////////////////////////////////////////
        // StringFilter
        //////////////////////////////////////////////////////////////////////////
        //! StringFilter filters assets based on their name
        class StringFilter
            : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            StringFilter();
            ~StringFilter() override = default;

            void SetFilterString(const QString& filterString);

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            QString m_filterString;
        };

        //////////////////////////////////////////////////////////////////////////
        // RegExpFilter
        //////////////////////////////////////////////////////////////////////////
        //! RegExpFilter filters assets based on a regular expression pattern
        class RegExpFilter
            : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            RegExpFilter();
            ~RegExpFilter() override = default;

            void SetFilterPattern(const QString& filterPattern);

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            QString m_filterPattern;
        };

        //////////////////////////////////////////////////////////////////////////
        // AssetTypeFilter
        //////////////////////////////////////////////////////////////////////////
        //! AssetTypeFilter filters products based on their asset id
        class AssetTypeFilter
            : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            AssetTypeFilter();
            ~AssetTypeFilter() override = default;

            void SetAssetType(AZ::Data::AssetType assetType);
            void SetAssetType(const char* assetTypeName);
            AZ::Data::AssetType GetAssetType() const;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            AZ::Data::AssetType m_assetType;
        };

        //////////////////////////////////////////////////////////////////////////
        // AssetGroupFilter
        //////////////////////////////////////////////////////////////////////////
        //! AssetGroupFilter filters products based on their asset group
        class AssetGroupFilter
            : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            AssetGroupFilter();
            ~AssetGroupFilter() override = default;

            void SetAssetGroup(const QString& group);
            const QString& GetAssetTypeGroup() const;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;

        private:
            QString m_group;
        };

        //////////////////////////////////////////////////////////////////////////
        // EntryTypeFilter
        //////////////////////////////////////////////////////////////////////////
        class EntryTypeFilter
            : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            EntryTypeFilter();
            ~EntryTypeFilter() override = default;

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
        class CompositeFilter
            : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            enum class LogicOperatorType
            {
                OR,
                AND
            };

            explicit CompositeFilter(LogicOperatorType logicOperator);
            ~CompositeFilter() override = default;

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
            void FilterInternal(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const override;

        private:
            QList<FilterConstType> m_subFilters;
            LogicOperatorType m_logicOperator;
            bool m_emptyResult;
        };

        //////////////////////////////////////////////////////////////////////////
        // InverseFilter
        //////////////////////////////////////////////////////////////////////////
        //! Inverse filter negates result of its child filter
        class InverseFilter
            : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            InverseFilter();
            ~InverseFilter() override = default;

            void SetFilter(FilterConstType filter);

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;
            void FilterInternal(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const override;

        private:
            FilterConstType m_filter;
        };

        //////////////////////////////////////////////////////////////////////////
        // CleanerProductsFilter
        //////////////////////////////////////////////////////////////////////////
        //! Filters out products that shouldn't be shown
        class CleanerProductsFilter
            : public AssetBrowserEntryFilter
        {
            Q_OBJECT
        public:
            CleanerProductsFilter();
            ~CleanerProductsFilter() override = default;

        protected:
            QString GetNameInternal() const override;
            bool MatchInternal(const AssetBrowserEntry* entry) const override;
            void FilterInternal(AZStd::vector<const AssetBrowserEntry*>& result, const AssetBrowserEntry* entry) const override;

        private:
            FilterConstType m_filter;
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
