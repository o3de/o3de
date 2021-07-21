/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
