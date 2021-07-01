/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        , mBar()
    {
    }


    // destructor
    ToolBarPlugin::~ToolBarPlugin()
    {
        if (!mBar.isNull())
        {
            EMStudio::GetMainWindow()->removeToolBar(mBar);
            delete mBar;
        }
    }

    void ToolBarPlugin::OnMainWindowClosed()
    {
        GetPluginManager()->RemoveActivePlugin(this);
    }

    // check if we have a window that uses this object name
    bool ToolBarPlugin::GetHasWindowWithObjectName(const AZStd::string& objectName)
    {
        if (mBar.isNull())
        {
            return false;
        }

        // check if the object name is equal to the one of the dock widget
        return objectName == FromQtString(mBar->objectName());
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
        if (!mBar.isNull())
        {
            mBar->setWindowTitle(name);
        }
    }

    QToolBar* ToolBarPlugin::GetToolBar()
    {
        if (!mBar.isNull())
        {
            return mBar;
        }

        MainWindow* mainWindow = GetMainWindow();

        // create the toolbar
        mBar = new QToolBar(GetName(), mainWindow);
        mBar->setAllowedAreas(GetAllowedAreas());
        mBar->setFloatable(GetIsFloatable());
        mBar->setMovable(GetIsMovable());
        mBar->setOrientation(GetIsVertical() ? Qt::Vertical : Qt::Horizontal);
        mBar->setToolButtonStyle(GetToolButtonStyle());

        // add the toolbar to the main window
        mainWindow->addToolBar(GetToolBarCreationArea(), mBar);

        return mBar;
    }

}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_ToolBarPlugin.cpp>
