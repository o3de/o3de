/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_TOOLBARPLUGIN_H
#define __EMSTUDIO_TOOLBARPLUGIN_H

// include MCore
#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include "EMStudioConfig.h"
#include "EMStudioPlugin.h"
#include <QToolBar>
#include <QPointer>
#endif


namespace EMStudio
{
    /**
     *
     *
     */
    class EMSTUDIO_API ToolBarPlugin
        : public EMStudioPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ToolBarPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)
    public:
        ToolBarPlugin();
        virtual ~ToolBarPlugin();

        EMStudioPlugin::EPluginType GetPluginType() const override          { return EMStudioPlugin::PLUGINTYPE_TOOLBAR; }
        void OnMainWindowClosed() override;

        virtual bool GetIsFloatable() const                                 { return true;  }
        virtual bool GetIsVertical() const                                  { return false; }
        virtual bool GetIsMovable() const                                   { return true;  }
        virtual Qt::ToolBarAreas GetAllowedAreas() const                    { return Qt::AllToolBarAreas; }
        virtual Qt::ToolButtonStyle GetToolButtonStyle() const              { return Qt::ToolButtonIconOnly; }

        virtual void SetInterfaceTitle(const char* name);
        void CreateBaseInterface(const char* objectName) override;

        QString GetObjectName() const override                      { AZ_Assert(!mBar.isNull(), "Unexpected null bar"); return mBar->objectName(); }
        void SetObjectName(const QString& name) override            { GetToolBar()->setObjectName(name); }

        bool GetHasWindowWithObjectName(const AZStd::string& objectName) override;

        virtual Qt::ToolBarArea GetToolBarCreationArea() const              { return Qt::BottomToolBarArea; }

        QToolBar* GetToolBar();

    protected:
        QPointer<QToolBar>   mBar;
    };
}   // namespace EMStudio

#endif
