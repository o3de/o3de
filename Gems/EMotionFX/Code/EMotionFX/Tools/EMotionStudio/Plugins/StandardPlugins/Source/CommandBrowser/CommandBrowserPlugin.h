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

#ifndef __EMSTUDIO_COMMANDBROWSERPLUGIN_H
#define __EMSTUDIO_COMMANDBROWSERPLUGIN_H
/*
// include MCore
//#include <MCore/Source/MCore.h"
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#endif

QT_FORWARD_DECLARE_CLASS(QWebView)


namespace EMStudio
{


class CommandBrowserPlugin : public EMStudio::DockWidgetPlugin
{
    MCORE_MEMORYOBJECTCATEGORY( CommandBrowserPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS );

    public:
        enum { CLASS_ID = 0x00000004 };

        CommandBrowserPlugin();
        ~CommandBrowserPlugin();

        // overloaded
        const char* GetCompileDate() const      { return MCORE_DATE; }
        const char* GetName() const             { return "Command Browser"; }
        uint32 GetClassID() const               { return CommandBrowserPlugin::CLASS_ID; }
        const char* GetCreatorName() const      { return "Amazon"; }
        float GetVersion() const                { return 1.0f;  }
        bool IsClosable() const                 { return true;  }
        bool IsFloatable() const                { return true;  }
        bool IsVertical() const                 { return false; }

        // overloaded main init function
        bool Init();
        EMStudioPlugin* Clone();

        void GenerateCommandList();

    private:
        QWebView*   mWebView;
};

} // namespace EMStudio
*/

#endif
