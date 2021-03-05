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
        , mDock()
    {
    }

    // destructor
    DockWidgetPlugin::~DockWidgetPlugin()
    {
        if (!mDock.isNull())
        {
            // Disconnecting all signals from mDock to this object since we are
            // destroying it. Some plugins connect to visibility change that gets
            // triggered from removeDockWidget. Calling those slots at this point
            // is not safe since the plugin is being destroyed.
            mDock->disconnect(this);

            EMStudio::GetMainWindow()->removeDockWidget(mDock);
            delete mDock;
        }
    }

    void DockWidgetPlugin::OnMainWindowClosed()
    {
        GetPluginManager()->RemoveActivePlugin(this);
    }

    // check if we have a window that uses this object name
    bool DockWidgetPlugin::GetHasWindowWithObjectName(const AZStd::string& objectName)
    {
        if (mDock.isNull())
        {
            return false;
        }

        // check if the object name is equal to the one of the dock widget
        return objectName == FromQtString(mDock->objectName());
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
        if (!mDock.isNull())
        {
            mDock->setWindowTitle(name);
        }
    }

    QDockWidget* DockWidgetPlugin::GetDockWidget()
    { 
        if (!mDock.isNull())
        {
            return mDock;
        }
        
        // get the main window
        QMainWindow* mainWindow = GetMainWindow();

        // create a window for the plugin
        mDock = new RemovePluginOnCloseDockWidget(mainWindow, GetName(), this);
        mDock->setAllowedAreas(Qt::AllDockWidgetAreas);

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

        mDock->setFeatures(features);

        //  mDock->setFloating( true );
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mDock);
        mainWindow->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North); // put tabs on top?

        return mDock;
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