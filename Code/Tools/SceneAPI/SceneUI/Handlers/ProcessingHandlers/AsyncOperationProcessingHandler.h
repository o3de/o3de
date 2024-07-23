#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ProcessingHandler.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <QThread>
#endif

namespace AZStd
{
    class thread;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class SCENE_UI_API AsyncOperationProcessingHandler : public ProcessingHandler
            {
                Q_OBJECT
            public:
                AsyncOperationProcessingHandler(Uuid traceTag, AZStd::function<void()> targetFunction, AZStd::function<void()> onComplete = nullptr, QObject* parent = nullptr);
                ~AsyncOperationProcessingHandler() override = default;
                void BeginProcessing() override;

            private Q_SLOTS:
                void OnBackgroundOperationComplete();

            private:
                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                AZStd::function<void()> m_operationToRun;
                AZStd::function<void()> m_onComplete;
                QThread* m_thread = nullptr;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
            };
        }
    }
}
