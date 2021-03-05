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

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
#endif

class QToolButton;

namespace AzQtComponents
{
    //! Displays custom buttons on the right side of the tab bar widget.
    class AZ_QT_COMPONENTS_API TabWidgetActionToolBar
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit TabWidgetActionToolBar(QWidget* parent = nullptr);

    signals:
        //! Triggered when the actions are changed.
        void actionsChanged();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        friend class TabWidget;
        friend class TabWidgetActionToolBarContainer;

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QHash<QAction*, QToolButton*> m_actionButtons;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

        void removeWidgetFromLayout(QWidget* widget);
    };

} // namespace AzQtComponents
