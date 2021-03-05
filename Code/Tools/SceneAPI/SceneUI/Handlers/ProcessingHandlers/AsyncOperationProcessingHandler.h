#pragma once

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

#if !defined(Q_MOC_RUN)
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ProcessingHandler.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
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

            private:
                void OnBackgroundOperationComplete();

            private:
                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                AZStd::function<void()> m_operationToRun;
                AZStd::function<void()> m_onComplete;
                AZStd::unique_ptr<AZStd::thread> m_thread;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
            };
        }
    }
}
