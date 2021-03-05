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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <SceneAPI/SceneUI/CommonWidgets/JobWatcher.h>
#endif

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class SCENE_UI_API ExportJobProcessingHandler : public ProcessingHandler
            {
                Q_OBJECT
            public:
                ExportJobProcessingHandler(Uuid traceTag, const AZStd::string& sourceAssetPath, QObject* parent = nullptr);
                ~ExportJobProcessingHandler() override = default;
                void BeginProcessing() override;

            private slots:
                void OnJobQueryFailed(const char* message);
                void OnJobProcessingComplete(const AZStd::string& platform, AZ::u64 jobId, bool success, const AZStd::string& fullLogText);
                void OnAllJobsComplete();

            private:
                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                AZStd::string m_sourceAssetPath;
                AZStd::unique_ptr<JobWatcher> m_jobWatcher;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
            };
        } // namespace SceneUI
    } //  namespace SceneAPI
} // namespace AZ
