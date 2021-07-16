#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

namespace AzToolsFramework
{
    namespace Logging
    {
        class LogEntry;
    }
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class SCENE_UI_API ProcessingHandler : public QObject
            {
                Q_OBJECT
            public:
                // The traceTag uuid is added to the trace context stack before work is done. This allows
                //      for filtering messages that are send by this processing handler.
                explicit ProcessingHandler(Uuid traceTag, QObject* parent = nullptr);
                ~ProcessingHandler() override = default;
                virtual void BeginProcessing() = 0;

            signals:
                void AddLogEntry(const AzToolsFramework::Logging::LogEntry& entry);

                void StatusMessageUpdated(const AZStd::string& message);
                
                void ProcessingComplete();

            protected:
                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                Uuid m_traceTag;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
            };
        }
    }
}
