/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LayerResult.h"

#include <QString>
#include <AzCore/Debug/Trace.h>

namespace AzToolsFramework
{
    namespace Layers
    {
        void LayerResult::MessageResult()
        {
            switch (m_result)
            {
            case AzToolsFramework::Layers::LayerResultStatus::Success:
                // Nothing to message on a success.
                break;
            case AzToolsFramework::Layers::LayerResultStatus::Error:
                AZ_Error("Layer", false, m_message.toUtf8().data());
                break;
            case AzToolsFramework::Layers::LayerResultStatus::Warning:
                AZ_Warning("Layer", false, m_message.toUtf8().data());
                break;
            default:
                AZ_Warning("Layer", false, "Unknown layer error.");
                AZ_Error("Layer", false, m_message.toUtf8().data());
                break;
            }
        }
    }
}
