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
