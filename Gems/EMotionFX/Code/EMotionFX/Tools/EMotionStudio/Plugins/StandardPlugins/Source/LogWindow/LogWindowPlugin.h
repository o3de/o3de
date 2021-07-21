/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_LOGWINDOWPLUGIN_H
#define __EMSTUDIO_LOGWINDOWPLUGIN_H

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#endif

namespace AzQtComponents
{
    struct SearchTypeFilter;
    class FilteredSearchWidget;
}

namespace EMStudio
{
    // forward declarations
    class LogWindowCallback;


    class LogWindowPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT // AUTOMOC
                           MCORE_MEMORYOBJECTCATEGORY(LogWindowPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000001
        };

        LogWindowPlugin();
        ~LogWindowPlugin();

        // overloaded
        const char* GetCompileDate() const override;
        const char* GetName() const override;
        uint32 GetClassID() const override;
        const char* GetCreatorName() const override;
        float GetVersion() const override;
        bool GetIsClosable() const override             { return true; }
        bool GetIsFloatable() const override            { return true; }
        bool GetIsVertical() const override             { return false; }
        bool Init() override;
        EMStudioPlugin* Clone() override;

    private slots:
        void OnTextFilterChanged(const QString& text);
        void OnTypeFilterChanged(const QVector<AzQtComponents::SearchTypeFilter>& filters);

    private:
        void AddFilter(const QString& name, MCore::LogCallback::ELogLevel level, bool enabled);

        LogWindowCallback* mLogCallback;
        AzQtComponents::FilteredSearchWidget* mSearchWidget;
    };
}   // namespace EMStudio

#endif
