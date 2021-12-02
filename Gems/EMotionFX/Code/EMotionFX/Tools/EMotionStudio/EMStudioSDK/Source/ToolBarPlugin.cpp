/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "EMStudioManager.h"
#include "MainWindow.h"
#include "ToolBarPlugin.h"


namespace EMStudio
{
    // constructor
    ToolBarPlugin::ToolBarPlugin()
        : EMStudioPlugin()
        , m_bar()
    {
    }


    // destructor
    ToolBarPlugin::~ToolBarPlugin()
    {
        if (!m_bar.isNull())
        {
            EMStudio::GetMainWindow()->removeToolBar(m_bar);
            delete m_bar;
        }
    }

    void ToolBarPlugin::OnMainWindowClosed()
    {
        GetPluginManager()->RemoveActivePlugin(this);
    }

    // check if we have a window that uses this object name
    bool ToolBarPlugin::GetHasWindowWithObjectName(const AZStd::string& objectName)
    {
        if (m_bar.isNull())
        {
            return false;
        }

        // check if the object name is equal to the one of the dock widget
        return objectName == FromQtString(m_bar->objectName());
    }


    // create the base interface
    void ToolBarPlugin::CreateBaseInterface(const char* objectName)
    {
        // set the object name
        if (objectName == nullptr)
        {
            SetObjectName(GetPluginManager()->GenerateObjectName());
        }
        else
        {
            SetObjectName(objectName);
        }
    }


    // set the interface title
    void ToolBarPlugin::SetInterfaceTitle(const char* name)
    {
        if (!m_bar.isNull())
        {
            m_bar->setWindowTitle(name);
        }
    }

    QToolBar* ToolBarPlugin::GetToolBar()
    {
        if (!m_bar.isNull())
        {
            return m_bar;
        }

        MainWindow* mainWindow = GetMainWindow();

        // create the toolbar
        m_bar = new QToolBar(GetName(), mainWindow);
        m_bar->setAllowedAreas(GetAllowedAreas());
        m_bar->setFloatable(GetIsFloatable());
        m_bar->setMovable(GetIsMovable());
        m_bar->setOrientation(GetIsVertical() ? Qt::Vertical : Qt::Horizontal);
        m_bar->setToolButtonStyle(GetToolButtonStyle());

        // add the toolbar to the main window
        mainWindow->addToolBar(GetToolBarCreationArea(), m_bar);

        return m_bar;
    }

}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_ToolBarPlugin.cpp>
