/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
