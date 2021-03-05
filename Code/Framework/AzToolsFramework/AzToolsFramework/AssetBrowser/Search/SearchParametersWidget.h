/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ_CLASS_ALLOCATOR(SearchParametersWidget, AZ::SystemAllocator, 0);

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
