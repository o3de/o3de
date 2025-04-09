/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/thread.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.h>

#include <QTimer>

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
                // Note that the use of a QThread instead of an AZStd::thread is intentional here, as signals, slots, timers, and other parts
                // of Qt will cause weird behavior and crashes if invoked from a non-QThread.  Qt tries its best to compensate, but without
                // a QThread as context, it may not correctly be able to invoke cross-thread event queues, or safely store objects in QThreadStorage.
                m_thread = QThread::create(
                    [this]()
                    {
                        AZ_TraceContext("Tag", m_traceTag);
                        m_operationToRun();
                        QMetaObject::invokeMethod(this, &AsyncOperationProcessingHandler::OnBackgroundOperationComplete, Qt::QueuedConnection);
                    }
                );
                m_thread->start();
            }

            void AsyncOperationProcessingHandler::OnBackgroundOperationComplete()
            {
                m_thread->quit(); // signal the thread's event pump to exit (at this point, its almost certainly already completed)
                m_thread->wait(); // wait for the thread to clean up any state, as well as actually join (ie, exit) so that it is no longer running.
                delete m_thread;
                m_thread = nullptr;

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
