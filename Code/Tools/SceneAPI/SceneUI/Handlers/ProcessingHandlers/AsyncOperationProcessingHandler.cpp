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

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/thread.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            AsyncOperationProcessingHandler::AsyncOperationProcessingHandler(Uuid traceTag, AZStd::function<void()> targetFunction, AZStd::function<void()> onComplete, QObject* parent)
                : ProcessingHandler(traceTag, parent)
                , m_operationToRun(targetFunction)
                , m_onComplete(onComplete)
            {
            }

            void AsyncOperationProcessingHandler::BeginProcessing()
            {
                emit StatusMessageUpdated("Waiting for background processes to complete...");
                m_thread.reset(
                    new AZStd::thread(
                        [this]()
                        {
                            AZ_TraceContext("Tag", m_traceTag);
                            m_operationToRun();
                            EBUS_QUEUE_FUNCTION(AZ::TickBus, AZStd::bind(&AsyncOperationProcessingHandler::OnBackgroundOperationComplete, this));
                        }
                    )
                );
            }

            void AsyncOperationProcessingHandler::OnBackgroundOperationComplete()
            {
                m_thread->detach();
                m_thread.reset(nullptr);

                emit StatusMessageUpdated("Processing complete");
                if (m_onComplete)
                {
                    m_onComplete();
                }
                emit ProcessingComplete();
            }
        }
    }
}

#include <Handlers/ProcessingHandlers/moc_AsyncOperationProcessingHandler.cpp>