/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

/*********************************************************************************************
 * SearchParametersWidget has been deprecated, use AzQtComponents::FilteredSearchWidget instead.
 *********************************************************************************************/

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <AzCore/Memory/SystemAllocator.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
                                                               // 4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QWidget>
#include <QScopedPointer>
AZ_POP_DISABLE_WARNING
#endif

namespace Ui
{
    class SearchParametersWidgetClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class SearchParametersWidget
            : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(SearchParametersWidget, AZ::SystemAllocator);

            explicit SearchParametersWidget(QWidget* parent = nullptr);
            ~SearchParametersWidget();


            void SetFilter(FilterConstType filter);
            void SetAllowClear(bool allowClear);

        Q_SIGNALS:
            void ClearAllSignal();

        private:
            QScopedPointer<Ui::SearchParametersWidgetClass> m_ui;
            FilterConstType m_filter;
            bool m_allowClear;

        private Q_SLOTS:
            void FilterUpdatedSlot();
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
