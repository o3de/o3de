/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
