/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

/*********************************************************************************************
 * FilterByWidget has been deprecated, use AzQtComponents::FilteredSearchWidget instead.
 *********************************************************************************************/

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>
#endif

namespace Ui
{
    class FilterByWidgetClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class FilterByWidget
            : public QWidget
        {
            Q_OBJECT
        public:
            explicit FilterByWidget(QWidget* parent = nullptr);
            ~FilterByWidget() override;
            void ToggleClearButton(bool visible) const;

        Q_SIGNALS:
            void ClearSignal();

        private:
            QScopedPointer<Ui::FilterByWidgetClass> m_ui;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
