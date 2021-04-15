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

#include "SearchParametersWidget.h"
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/AssetBrowser/Search/ui_SearchParametersWidget.h>
AZ_POP_DISABLE_WARNING
#include <AzQtComponents/Components/ExtendedLabel.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        SearchParametersWidget::SearchParametersWidget(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::SearchParametersWidgetClass())
            , m_allowClear(true)
        {
            m_ui->setupUi(this);
            hide();
            connect(m_ui->m_clearFiltersButton, &AzQtComponents::ExtendedLabel::clicked, this, &SearchParametersWidget::ClearAllSignal);
        }

        SearchParametersWidget::~SearchParametersWidget() = default;

        void SearchParametersWidget::FilterUpdatedSlot()
        {
            QString filterName = m_filter->GetName();
            if (!filterName.isEmpty())
            {
                show();
                m_ui->m_filtersLabel->setText("<b>Filtered by:</b> " + filterName);
                if (m_allowClear)
                {
                    m_ui->m_clearFiltersButton->show();
                }
                else
                {
                    m_ui->m_clearFiltersButton->hide();
                }
            }
            else
            {
                hide();
            }
        }

        void SearchParametersWidget::SetFilter(FilterConstType filter)
        {
            m_filter = filter;
            connect(m_filter.data(), &AssetBrowserEntryFilter::updatedSignal, this, &SearchParametersWidget::FilterUpdatedSlot);
        }

        void SearchParametersWidget::SetAllowClear(bool allowClear)
        {
            m_allowClear = allowClear;
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Search/moc_SearchParametersWidget.cpp"
