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

#include "EditorCommonAPI.h"
#include "IEditor.h"


#include <WinWidget/WinWidgetManager.h>
#include <Core/QtEditorApplication.h>
#include <QWidget>

namespace WinWidget
{
    template<class TWidget>
    bool RegisterWinWidget()
    {
        static QWidget* winWidget {nullptr}; // Must declare outside of lambda

        WinWidget::WinWidgetManager::WinWidgetCreateCall createCall = []() -> QWidget*
            {
                if (!winWidget)
                {
                    winWidget = new QWidget(GetIEditor()->GetEditorMainWindow());
                }
                
                // Ensure only one instance of each window type exists
                QList<TWidget*> existingWidgets = winWidget->findChildren<TWidget*>();
                if (existingWidgets.size() > 0) // Note that the list should contain 0 or 1 entries
                { 
                    if (existingWidgets.first()->isVisible())
                    {
                        return nullptr; // TWidget type already in use - continue using it and don't create another
                    }
                    delete existingWidgets.first(); // Closed TWidget - remove
                }

                TWidget* createWidget = new TWidget(winWidget);

                createWidget->Display();
                return winWidget;
            };

        return GetIEditor()->GetWinWidgetManager()->RegisterWinWidget(TWidget::GetWWId(), createCall);
    }

    template<class TWidget>
    void UnregisterWinWidget()
    {
        GetIEditor()->GetWinWidgetManager()->UnregisterWinWidget(TWidget::GetWWId());
    }
}
