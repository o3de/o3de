/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMStudioManager.h"
#include "MainWindow.h"
#include "DockWidgetPlugin.h"
#include "RemovePluginOnCloseDockWidget.h"
#include <MCore/Source/LogManager.h>
#include <QHBoxLayout>


namespace EMStudio
{
    // constructor
    DockWidgetPlugin::DockWidgetPlugin()
        : EMStudioPlugin()
        , m_dock()
    {
    }

    // destructor
    DockWidgetPlugin::~DockWidgetPlugin()
    {
        if (!m_dock.isNull())
        {
            // Disconnecting all signals from m_dock to this object since we are
            // destroying it. Some plugins connect to visibility change that gets
            // triggered from removeDockWidget. Calling those slots at this point
            // is not safe since the plugin is being destroyed.
            m_dock->disconnect(this);

            EMStudio::GetMainWindow()->removeDockWidget(m_dock);
            delete m_dock;
        }
    }

    void DockWidgetPlugin::OnMainWindowClosed()
    {
        GetPluginManager()->RemoveActivePlugin(this);
    }

    // check if we have a window that uses this object name
    bool DockWidgetPlugin::GetHasWindowWithObjectName(const AZStd::string& objectName)
    {
        if (m_dock.isNull())
        {
            return false;
        }

        // check if the object name is equal to the one of the dock widget
        return objectName == FromQtString(m_dock->objectName());
    }


    // create the base interface
    void DockWidgetPlugin::CreateBaseInterface(const char* objectName)
    {
        if (!objectName)
        {
            QString newName = GetPluginManager()->GenerateObjectName();
            SetObjectName(newName);
        }
        else
        {
            SetObjectName(objectName);
        }
    }


    // set the interface title
    void DockWidgetPlugin::SetInterfaceTitle(const char* name)
    {
        if (!m_dock.isNull())
        {
            m_dock->setWindowTitle(name);
        }
    }

    QDockWidget* DockWidgetPlugin::GetDockWidget()
    { 
        if (!m_dock.isNull())
        {
            return m_dock;
        }
        
        // get the main window
        QMainWindow* mainWindow = GetMainWindow();

        // create a window for the plugin
        m_dock = new RemovePluginOnCloseDockWidget(mainWindow, GetName(), this);
        m_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

        QDockWidget::DockWidgetFeatures features = QDockWidget::NoDockWidgetFeatures;
        if (GetIsClosable())
        {
            features |= QDockWidget::DockWidgetClosable;
        }
        if (GetIsVertical())
        {
            features |= QDockWidget::DockWidgetVerticalTitleBar;
        }
        if (GetIsMovable())
        {
            features |= QDockWidget::DockWidgetMovable;
        }
        if (GetIsFloatable())
        {
            features |= QDockWidget::DockWidgetFloatable;
        }

        m_dock->setFeatures(features);

        mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_dock);

        return m_dock;
    }

    QWidget* DockWidgetPlugin::CreateErrorContentWidget(const char* errorMessage) const
    {
        QWidget* widget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setMargin(32);
        widget->setLayout(layout);

        QLabel* label = new QLabel(errorMessage);
        label->setWordWrap(true);
        layout->addWidget(label);

        return widget;
    }
} // namespace EMStudio
