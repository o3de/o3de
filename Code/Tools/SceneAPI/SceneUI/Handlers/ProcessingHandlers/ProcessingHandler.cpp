/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ProcessingHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            ProcessingHandler::ProcessingHandler(Uuid traceTag, QObject* parent)
                : QObject(parent)
                , m_traceTag(traceTag)
            {
            }
        }
    }
}

#include <Handlers/ProcessingHandlers/moc_ProcessingHandler.cpp>
