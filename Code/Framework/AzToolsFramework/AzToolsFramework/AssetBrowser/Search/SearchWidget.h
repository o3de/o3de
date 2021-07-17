/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
AZ_POP_DISABLE_WARNING

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <QSharedPointer>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class SearchWidget
            : public AzQtComponents::FilteredSearchWidget
        {
            Q_OBJECT

        public:
            explicit SearchWidget(QWidget* parent = nullptr);

            void Setup(bool stringFilter, bool assetTypeFilter);

            QSharedPointer<CompositeFilter> GetFilter() const;

            QSharedPointer<CompositeFilter> GetStringFilter() const;

            QSharedPointer<CompositeFilter> GetTypesFilter() const;

            QString GetFilterString() const { return textFilter(); }
            void ClearStringFilter() { ClearTextFilter(); }

        private:
            QSharedPointer<CompositeFilter> m_filter;
            QSharedPointer<CompositeFilter> m_stringFilter;
            QSharedPointer<CompositeFilter> m_typesFilter;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
