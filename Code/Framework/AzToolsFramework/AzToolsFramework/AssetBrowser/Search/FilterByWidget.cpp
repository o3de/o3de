/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/ExtendedLabel.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/AssetBrowser/Search/ui_FilterByWidget.h>
AZ_POP_DISABLE_WARNING
#include <AzToolsFramework/AssetBrowser/Search/FilterByWidget.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        FilterByWidget::FilterByWidget(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::FilterByWidgetClass)
        {
            m_ui->setupUi(this);
            connect(m_ui->m_clearFiltersButton, &AzQtComponents::ExtendedLabel::clicked, this, &FilterByWidget::ClearSignal);
            // hide clear button as filters are reset at the startup
            ToggleClearButton(false);
        }

        FilterByWidget::~FilterByWidget() = default;

        void FilterByWidget::ToggleClearButton(bool visible) const
        {
            m_ui->m_clearFiltersButton->setVisible(visible);
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Search/moc_FilterByWidget.cpp"
