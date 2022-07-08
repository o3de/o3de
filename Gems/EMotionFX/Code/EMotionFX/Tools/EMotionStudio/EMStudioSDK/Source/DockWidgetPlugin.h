/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include "EMStudioConfig.h"
#include "EMStudioPlugin.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <QPointer>
#endif


namespace EMStudio
{
    class EMSTUDIO_API DockWidgetPlugin
        : public EMStudioPlugin
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(DockWidgetPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        DockWidgetPlugin();
         ~DockWidgetPlugin() override;

        EMStudioPlugin::EPluginType GetPluginType() const override              { return EMStudioPlugin::PLUGINTYPE_WINDOW; }

        void OnMainWindowClosed() override;

        virtual bool GetIsClosable() const                                  { return true; }
        virtual bool GetIsFloatable() const                                 { return true;  }
        virtual bool GetIsVertical() const                                  { return false; }
        virtual bool GetIsMovable() const                                   { return true;  }

        virtual void SetInterfaceTitle(const char* name);
        void CreateBaseInterface(const char* objectName) override;

        QString GetObjectName() const override                      { AZ_Assert(m_dock, "m_dock is null"); return m_dock->objectName(); }
        void SetObjectName(const QString& name) override            { GetDockWidget()->setObjectName(name); }

        virtual QSize GetInitialWindowSize() const                          { return QSize(500, 650); }

        bool GetHasWindowWithObjectName(const AZStd::string& objectName) override;

        QDockWidget* GetDockWidget();

    protected:
        QWidget* CreateErrorContentWidget(const char* errorMessage) const;

        QPointer<QDockWidget> m_dock;
    };
}   // namespace EMStudio
