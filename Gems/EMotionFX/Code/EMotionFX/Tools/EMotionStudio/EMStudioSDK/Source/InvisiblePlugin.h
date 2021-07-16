/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_INVISIBLEPLUGIN_H
#define __EMSTUDIO_INVISIBLEPLUGIN_H

// include MCore
#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include "EMStudioConfig.h"
#include "EMStudioPlugin.h"
#endif


namespace EMStudio
{
    /**
     *
     *
     */
    class EMSTUDIO_API InvisiblePlugin
        : public EMStudioPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(InvisiblePlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)
    public:
        InvisiblePlugin();
        virtual ~InvisiblePlugin();

        EMStudioPlugin::EPluginType GetPluginType() const override              { return EMStudioPlugin::PLUGINTYPE_INVISIBLE; }

        bool Init() override { return true; }   // for this type of plugin, perform the init inside the constructor
        bool GetHasWindowWithObjectName(const AZStd::string& objectName) override { MCORE_UNUSED(objectName); return false; }
        QString GetObjectName() const override                          { return objectName(); }
        void SetObjectName(const QString& objectName) override          { setObjectName(objectName); }
        void CreateBaseInterface(const char* objectName) override       { MCORE_UNUSED(objectName); }
    };
}   // namespace EMStudio

#endif
