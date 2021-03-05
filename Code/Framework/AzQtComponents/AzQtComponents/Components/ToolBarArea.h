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

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QMainWindow>

class QToolBar;
class QVBoxLayout;

namespace AzQtComponents
{
    // ToolBarArea inherits from QMainWindow to offer a docking area for QToolBars
    // in which they can expand when they lack the horizontal space to be fully shown
    class AZ_QT_COMPONENTS_API ToolBarArea : public QMainWindow
    {
    public:
        ToolBarArea(QWidget* parent);

    protected:
        // Helper method to turn a source QWidget with a layout into a QToolBar
        // Grabs all widgets from the source widget's layout and adds spacers for any stretch in the layout
        // Does NOT delete sourceWidget
        QToolBar* CreateToolBarFromWidget(QWidget* sourceWidget, Qt::ToolBarArea area = Qt::TopToolBarArea, QString title = {});

        QWidget* GetMainWidget() { return m_mainWidget; }

        // Sets the primary widget in the center of the ToolBarArea
        // Differs from QMainWindow::setCentralWidget in that it will not delete a previous main widget
        void SetMainWidget(QWidget* widget);

    private:
        QVBoxLayout* m_mainLayout = nullptr;
        QWidget* m_mainWidget = nullptr;
    };
}
